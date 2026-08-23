import { createTest, expect } from '@ngx-playwright/test';

import { ApplicationScreen } from '../screens/main-screen.js';

const test = createTest(ApplicationScreen);

type BootstrapState = {
  active: boolean;
  screen: 'entry' | 'create' | 'join' | 'lobby' | 'error';
  connected: boolean;
  busy: boolean;
  joinCode: string;
  canStart: boolean;
  members: { name: string; present: boolean }[];
  error: string;
};

const entryState: BootstrapState = {
  active: true,
  screen: 'entry',
  connected: false,
  busy: false,
  joinCode: '',
  canStart: false,
  members: [],
  error: '',
};

async function project(page: any, state: BootstrapState): Promise<void> {
  await page.evaluate((nextState: BootstrapState) => {
    const bridge = (globalThis as any).skyrimtogether;
    bridge.emit('uiSurface', 'campaignBootstrap');
    bridge.emit('campaignBootstrapState', JSON.stringify(nextState));
  }, state);
}

test.describe('Campaign bootstrap', () => {
  test.beforeEach(async ({ page }) => {
    await page.waitForSelector('app-root');
    await page.evaluate(() => {
      const bridge = (globalThis as any).skyrimtogether;
      (globalThis as any).__campaignActions = [];
      bridge.campaignBootstrapAction = (...args: string[]) => {
        (globalThis as any).__campaignActions.push(args);
      };
    });
    await project(page, entryState);
    await page.waitForSelector('.bootstrap-shell');
  });

  test('entry emits only the selected Solo/Create/Join intent', async ({
    page,
  }) => {
    const buttons = page.locator('.entry-actions button');
    await expect(buttons).toHaveCount(3);
    await buttons.nth(0).click();
    await buttons.nth(1).click();
    await buttons.nth(2).click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([
        ['solo', '', '', '', ''],
        ['showCreate', '', '', '', ''],
        ['showJoin', '', '', '', ''],
      ]);
  });

  test('connected Create and Join forms hide only connection fields', async ({
    page,
  }) => {
    await project(page, { ...entryState, screen: 'create', connected: true });
    await expect(page.locator('input[name="pseudo"]')).toHaveCount(1);
    await expect(page.locator('input[name="address"]')).toHaveCount(0);
    await expect(page.locator('input[name="password"]')).toHaveCount(0);

    await project(page, { ...entryState, screen: 'join', connected: true });
    await expect(page.locator('input[name="pseudo"]')).toHaveCount(1);
    await expect(page.locator('input[name="code"]')).toHaveCount(1);
    await expect(page.locator('input[name="address"]')).toHaveCount(0);
  });

  test('join code uppercases and removes ambiguous glyphs', async ({
    page,
  }) => {
    await project(page, { ...entryState, screen: 'join', connected: true });
    await page.locator('input[name="pseudo"]').fill('  Éowyn  ');
    const code = page.locator('input[name="code"]');
    await code.fill('a1i7k2');
    await expect(code).toHaveValue('A7K2');
    await page.locator('form button.primary').click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([['join', '', '', 'A7K2', 'Éowyn']]);
  });

  test('pseudo is required, bounded, control-free, trimmed, and Unicode-safe', async ({
    page,
  }) => {
    await project(page, { ...entryState, screen: 'create', connected: true });
    const pseudo = page.locator('input[name="pseudo"]');
    const submit = page.locator('form button.primary');
    await expect(submit).toBeDisabled();

    await pseudo.evaluate((input: HTMLInputElement) => {
      input.value = 'Player\u0001Two';
      input.dispatchEvent(new Event('input', { bubbles: true }));
    });
    await expect(submit).toBeDisabled();

    await pseudo.evaluate((input: HTMLInputElement) => {
      input.value = 'x'.repeat(25);
      input.dispatchEvent(new Event('input', { bubbles: true }));
    });
    await expect(submit).toBeDisabled();

    await pseudo.fill('  Léa 🐉  ');
    await expect(submit).toBeEnabled();
    await submit.click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([['create', '', '', '', 'Léa 🐉']]);
  });

  test('Create and Join share the regular persisted connection address without storing passwords', async ({
    page,
  }) => {
    await page.evaluate(() => {
      localStorage.setItem('last_connected_address', 'saved.example:10579');
      localStorage.setItem('last_connected_password', 'leave-unchanged');
    });
    await page.reload();
    await page.waitForSelector('app-root');
    await page.evaluate(() => {
      const bridge = (globalThis as any).skyrimtogether;
      (globalThis as any).__campaignActions = [];
      bridge.campaignBootstrapAction = (...args: string[]) => {
        (globalThis as any).__campaignActions.push(args);
      };
    });

    await project(page, { ...entryState, screen: 'create' });
    await page.locator('input[name="pseudo"]').fill('Creator');
    const address = page.locator('input[name="address"]');
    await expect(address).toHaveValue('saved.example:10579');
    await address.fill('shared.example:10600');
    await page.locator('input[name="password"]').fill('not-persisted');
    await page.locator('form button.primary').click();
    await expect
      .poll(() =>
        page.evaluate(() => localStorage.getItem('last_connected_address')),
      )
      .toBe('shared.example:10600');
    await expect
      .poll(() =>
        page.evaluate(() => localStorage.getItem('last_connected_password')),
      )
      .toBe('leave-unchanged');

    await project(page, { ...entryState, screen: 'join' });
    await expect(page.locator('input[name="address"]')).toHaveValue(
      'shared.example:10600',
    );
    await page.locator('input[name="address"]').fill('joined.example');
    await page.locator('input[name="code"]').fill('A7K2');
    await page.locator('form button.primary').click();
    await expect
      .poll(() =>
        page.evaluate(() => localStorage.getItem('last_connected_address')),
      )
      .toBe('joined.example');
    await expect
      .poll(() =>
        page.evaluate(() => localStorage.getItem('last_connected_password')),
      )
      .toBe('leave-unchanged');
  });

  test('lobby projects names and derives Start visibility and busy state', async ({
    page,
  }) => {
    const lobby: BootstrapState = {
      ...entryState,
      screen: 'lobby',
      connected: true,
      joinCode: 'R5WT',
      members: [
        { name: 'Matthieu', present: true },
        { name: 'Alice', present: false },
      ],
    };
    await project(page, lobby);
    await expect(page.locator('.join-code')).toHaveText('R5WT');
    await expect(page.locator('.lobby li')).toHaveCount(2);
    await expect(page.locator('.lobby li').nth(1)).toHaveClass(/absent/);
    await expect(page.locator('.lobby button.start')).toHaveCount(0);

    await project(page, { ...lobby, canStart: true, busy: true });
    await expect(page.locator('.lobby button.start')).toBeDisabled();
    await project(page, { ...lobby, canStart: true, busy: false });
    await page.locator('.lobby button.start').click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([['start', '', '', '', '']]);
  });

  test('Escape navigates forms but cannot bypass entry or lobby', async ({
    page,
  }) => {
    await page.keyboard.press('Escape');
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([]);

    await project(page, { ...entryState, screen: 'create' });
    await page.keyboard.press('Escape');
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([['back', '', '', '', '']]);

    await page.evaluate(() => ((globalThis as any).__campaignActions = []));
    await project(page, {
      ...entryState,
      screen: 'lobby',
      connected: true,
      joinCode: 'A7K2',
      members: [{ name: 'Matthieu', present: true }],
    });
    await page.keyboard.press('Escape');
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__campaignActions))
      .toEqual([]);
  });
});
