import { createTest, expect } from '@ngx-playwright/test';

import { ApplicationScreen } from '../screens/main-screen.js';

const test = createTest(ApplicationScreen);

async function project(page: any, inCampaign: boolean): Promise<void> {
  await page.evaluate((value: boolean) => {
    (globalThis as any).skyrimtogether.emit(
      'campaignSavePolicyState',
      JSON.stringify({ inCampaign: value }),
    );
  }, inCampaign);
}

test.describe('Campaign save policy projection', () => {
  test.beforeEach(async ({ page }) => {
    await page.waitForSelector('app-root');
    await page.evaluate(() => {
      const bridge = (globalThis as any).skyrimtogether;
      bridge.emit('enterGame');
      bridge.emit('uiSurface', 'str');
      bridge.emit('connect');
    });
    await page.press('body', 'F2');
    await page.waitForSelector('.app-root-controls', { state: 'visible' });
    await page.getByText('Settings', { exact: true }).click();
    await page.waitForSelector('app-settings');
  });

  test('projects all supported autosave controls as disabled only in campaign', async ({
    page,
  }) => {
    await project(page, false);
    await expect(page.getByTestId('campaign-save-policy')).toHaveCount(0);

    await project(page, true);
    const policy = page.getByTestId('campaign-save-policy');
    await expect(policy).toBeVisible();
    await expect(policy.locator('input[type="checkbox"]')).toHaveCount(4);
    for (let index = 0; index < 4; ++index) {
      await expect(policy.locator('input').nth(index)).toBeDisabled();
      await expect(policy.locator('input').nth(index)).not.toBeChecked();
    }
    await expect(policy).toContainText('Disabled in multiplayer');

    await project(page, false);
    await expect(page.getByTestId('campaign-save-policy')).toHaveCount(0);
  });

  test('localizes requested committed and failed checkpoint outcomes', async ({
    page,
  }) => {
    await page.getByText('Back', { exact: true }).click();
    for (const key of [
      'COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_REQUESTED',
      'COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_COMMITTED',
      'COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_FAILED',
    ]) {
      await page.evaluate((translationKey: string) => {
        (globalThis as any).skyrimtogether.emit(
          'message',
          0,
          translationKey,
        );
      }, key);
    }

    const chat = page.locator('app-chat');
    await expect(
      chat.getByText('Creating the collective checkpoint...', {
        exact: true,
      }),
    ).toBeVisible();
    await expect(
      chat.getByText('Collective checkpoint created.', { exact: true }),
    ).toBeVisible();
    await expect(
      chat.getByText('Unable to create the collective checkpoint.', {
        exact: true,
      }),
    ).toBeVisible();
  });
});
