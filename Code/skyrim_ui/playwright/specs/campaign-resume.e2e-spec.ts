import { createTest, expect } from '@ngx-playwright/test';

import { ApplicationScreen } from '../screens/main-screen.js';

const test = createTest(ApplicationScreen);

type ResumeState = {
  phase:
    | 'unavailable'
    | 'ready'
    | 'submitting'
    | 'admitted'
    | 'waitingForRoster'
    | 'recovery'
    | 'synchronizing'
    | 'active'
    | 'error';
  connected: boolean;
  resumeRequired: boolean;
  disconnectIncident: boolean;
  disconnectRecovery: boolean;
  incidentKind:
    | 'remotePlayerMissing'
    | 'multiplePlayersMissing'
    | 'localTransportLost'
    | 'rosterRestored';
  missingMembers: number;
  loadedSaveValid: boolean;
  selectedToken: string;
  candidates: { token: string; ordinal: number }[];
  roster: { ordinal: number; present: boolean; local: boolean }[];
  error: string;
};

const tokenA = '00112233445566778899aabbccddeeff';
const tokenB = 'ffeeddccbbaa99887766554433221100';

const regularReady: ResumeState = {
  phase: 'ready',
  connected: true,
  resumeRequired: false,
  disconnectIncident: false,
  disconnectRecovery: false,
  incidentKind: 'rosterRestored',
  missingMembers: 0,
  loadedSaveValid: false,
  selectedToken: '',
  candidates: [],
  roster: [],
  error: '',
};

const saveReady: ResumeState = {
  ...regularReady,
  resumeRequired: true,
  loadedSaveValid: true,
  candidates: [{ token: tokenA, ordinal: 1 }],
};

const remoteIncident: ResumeState = {
  ...regularReady,
  phase: 'waitingForRoster',
  disconnectIncident: true,
  incidentKind: 'remotePlayerMissing',
  missingMembers: 1,
  roster: [
    { ordinal: 1, present: true, local: true },
    { ordinal: 2, present: false, local: false },
  ],
};

async function project(page: any, state: ResumeState): Promise<void> {
  await page.evaluate((nextState: ResumeState) => {
    (globalThis as any).skyrimtogether.emit(
      'campaignResumeState',
      JSON.stringify(nextState),
    );
  }, state);
}

async function showMandatory(page: any, state = saveReady): Promise<void> {
  await project(page, state);
  await page.waitForSelector('app-campaign-resume .campaign-shell');
}

async function openRegular(page: any): Promise<void> {
  await page.getByText('Resume campaign', { exact: true }).click();
  await page.waitForSelector('app-campaign-resume .campaign-shell');
  await project(page, regularReady);
}

test.describe('Campaign resume', () => {
  test.beforeEach(async ({ page }) => {
    await page.waitForSelector('app-root');
    await page.evaluate(() => {
      const bridge = (globalThis as any).skyrimtogether;
      (globalThis as any).__resumeActions = [];
      bridge.campaignResumeAction = (...args: string[]) => {
        (globalThis as any).__resumeActions.push(args);
      };
      bridge.emit('enterGame');
      bridge.emit('uiSurface', 'str');
      bridge.emit('connect');
    });
    await page.press('body', 'F2');
    await page.waitForSelector('.app-root-controls', { state: 'visible' });
  });

  test('regular cold-session resume keeps multiple opaque candidates explicit and idempotent', async ({
    page,
  }) => {
    await openRegular(page);
    await project(page, {
      ...regularReady,
      candidates: [
        { token: tokenA, ordinal: 1 },
        { token: tokenB, ordinal: 2 },
      ],
    });

    const candidates = page.locator('.candidate-list button');
    await expect(candidates).toHaveCount(2);
    await candidates.nth(1).dblclick();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['refresh'], ['select', tokenB]]);
  });

  test('remote disconnect incident is explicit and Stay reuses the existing recovery component', async ({
    page,
  }) => {
    await project(page, remoteIncident);
    const component = page.locator('app-campaign-resume');
    await expect(component).toHaveCount(1);
    await expect(component).toContainText('A PLAYER LEFT THE GAME');
    await expect(component).toContainText(
      'The campaign cannot continue until every required player is present.',
    );
    await expect(component).toContainText('Players present: 1 / 2');
    await expect(component.getByText('Player 2')).toBeVisible();
    await expect(
      component.getByRole('button', { name: 'LOAD THE LAST CHECKPOINT' }),
    ).toBeVisible();
    await expect(
      component.getByRole('button', { name: 'RETURN TO MAIN MENU' }),
    ).toBeVisible();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([]);

    await component
      .getByRole('button', { name: 'LOAD THE LAST CHECKPOINT' })
      .dblclick();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['stayAndRecover']]);

    await project(page, {
      ...remoteIncident,
      disconnectIncident: false,
      disconnectRecovery: true,
    });
    await expect(page.locator('app-campaign-resume')).toHaveCount(1);
    await expect(component).toContainText('CAMPAIGN WAITING');
    await expect(component).toContainText('Players present: 1 / 2');

    await project(page, {
      ...remoteIncident,
      phase: 'recovery',
      disconnectIncident: false,
      disconnectRecovery: true,
      incidentKind: 'rosterRestored',
      missingMembers: 0,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: true, local: false },
      ],
    });
    await expect(component).toContainText(
      'Loading the last collective checkpoint',
    );

    await project(page, {
      ...remoteIncident,
      phase: 'synchronizing',
      disconnectIncident: false,
      disconnectRecovery: true,
      incidentKind: 'rosterRestored',
      missingMembers: 0,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: true, local: false },
      ],
    });
    await expect(component).toContainText(
      'Applying the authoritative campaign state',
    );
  });

  test('incident identity follows one, several, restored and local-loss semantics without stale names', async ({
    page,
  }) => {
    await project(page, {
      ...remoteIncident,
      incidentKind: 'multiplePlayersMissing',
      missingMembers: 2,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: false, local: false },
        { ordinal: 3, present: false, local: false },
      ],
    });
    const component = page.locator('app-campaign-resume');
    await expect(component).toContainText('SEVERAL PLAYERS LEFT THE GAME');

    await project(page, {
      ...remoteIncident,
      incidentKind: 'rosterRestored',
      missingMembers: 0,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: true, local: false },
      ],
    });
    await expect(component).toContainText('ROSTER RESTORED');
    await expect(component).not.toContainText('SEVERAL PLAYERS LEFT');

    await project(page, {
      ...remoteIncident,
      connected: false,
      incidentKind: 'localTransportLost',
      missingMembers: 0,
      roster: [{ ordinal: 1, present: true, local: true }],
    });
    await expect(component).toContainText('CONNECTION TO THE GAME LOST');
    await expect(component).not.toContainText('A PLAYER LEFT THE GAME');
  });

  test('Return to Main Menu emits only the bounded native lifecycle action', async ({
    page,
  }) => {
    const diagnostics: string[] = [];
    page.on('console', message => diagnostics.push(message.text()));
    await project(page, remoteIncident);
    const button = page
      .locator('app-campaign-resume')
      .getByRole('button', { name: 'RETURN TO MAIN MENU' });
    await button.dblclick();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['returnToMainMenu']]);
    await expect
      .poll(() => diagnostics)
      .toContain(
        '[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_CLICKED',
      );
    await expect
      .poll(() => diagnostics)
      .toContain(
        '[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_BRIDGE_SEND',
      );
  });

  test('authoritative disconnect recovery completion closes the reused mandatory surface', async ({
    page,
  }) => {
    await project(page, {
      ...remoteIncident,
      phase: 'synchronizing',
      disconnectIncident: false,
      disconnectRecovery: true,
      incidentKind: 'rosterRestored',
      missingMembers: 0,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: true, local: false },
      ],
    });
    await expect(page.locator('app-campaign-resume')).toHaveCount(1);
    await project(page, regularReady);
    await expect(page.locator('app-campaign-resume')).toHaveCount(0);
  });

  test('campaign save detection is mandatory and offers no solo continuation', async ({
    page,
  }) => {
    await showMandatory(page);
    const surface = page.locator('app-campaign-resume');
    await expect(surface).toContainText('MULTIPLAYER CAMPAIGN');
    await expect(surface).toContainText(
      'This save belongs to an STRE campaign.',
    );
    await expect(surface).toContainText('Local save verified');
    await expect(surface).toContainText('Connect to the server');
    await expect(surface).toContainText('Verify the campaign');
    await expect(surface.locator('.candidate-list button')).toHaveCount(0);
    await expect(
      surface.getByRole('button', { name: 'RESUME CAMPAIGN' }),
    ).toBeVisible();
    await expect(
      surface.getByRole('button', { name: 'RETURN TO MAIN MENU' }),
    ).toBeDisabled();
    await expect(surface.getByText(/continue in solo/i)).toHaveCount(0);
    await expect(surface.getByText(/^solo$/i)).toHaveCount(0);
    await expect(surface.getByRole('button', { name: 'CLOSE' })).toHaveCount(0);
  });

  test('a disconnected campaign save reuses the existing connection form without a cancel bypass', async ({
    page,
  }) => {
    await showMandatory(page, { ...saveReady, connected: false });
    const connection = page.locator('app-campaign-resume app-connect');
    await expect(connection).toBeVisible();
    await expect(
      connection.getByRole('button', { name: /^cancel$/i }),
    ).toHaveCount(0);
    await expect(
      page
        .locator('app-campaign-resume')
        .getByRole('button', { name: 'RESUME CAMPAIGN' }),
    ).toHaveCount(0);
    await connection.getByPlaceholder('Address').fill('campaign.test');
    await connection.getByRole('button', { name: /^connect$/i }).click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['refresh'], ['refresh']]);
  });

  test('loaded campaign X receives only its exact opaque target', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      candidates: [{ token: tokenA, ordinal: 1 }],
    });
    const resume = page
      .locator('app-campaign-resume')
      .getByRole('button', { name: 'RESUME CAMPAIGN' });
    await expect(resume).toHaveCount(1);
    await expect(
      page.locator('app-campaign-resume .candidate-list button'),
    ).toHaveCount(0);
    await expect(page.getByText('Campaign 2', { exact: true })).toHaveCount(0);
    await resume.click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['refresh'], ['select', tokenA]]);
  });

  test('connection and authoritative admission progress never expose a local success action', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'submitting',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Requesting authoritative admission',
    );
    await expect(
      page.locator('app-campaign-resume button:not([disabled])'),
    ).toHaveCount(0);

    await project(page, {
      ...saveReady,
      phase: 'admitted',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Admission accepted',
    );
  });

  test('WAITING_FOR_ROSTER uses the shared roster and authoritative presence', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'waitingForRoster',
      selectedToken: tokenA,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: false, local: false },
      ],
    });
    const roster = page.locator('app-campaign-resume app-campaign-roster');
    await expect(roster).toContainText('Players present: 1 / 2');
    await expect(roster).toContainText('You');
    await expect(roster).toContainText('Player 2');
    await expect(roster.locator('li').nth(1)).toHaveClass(/absent/);
  });

  test('recovery and authoritative snapshot synchronization are distinct phases', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'recovery',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Loading the last collective checkpoint',
    );

    await project(page, {
      ...saveReady,
      phase: 'synchronizing',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Applying the authoritative campaign state',
    );
  });

  test('fail-closed errors keep the gate UX, explain the reason and offer retry', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'error',
      loadedSaveValid: false,
      candidates: [],
      error: 'save_marker_invalid',
    });
    const surface = page.locator('app-campaign-resume');
    await expect(surface).toContainText('UNABLE TO RESUME CAMPAIGN');
    await expect(surface).toContainText('marker is invalid');
    await surface.getByRole('button', { name: 'TRY AGAIN' }).click();
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['refresh'], ['retry']]);
    await expect(
      surface.getByRole('button', { name: 'RETURN TO MAIN MENU' }),
    ).toBeDisabled();
  });

  test('F2 close and reopen preserves mandatory state without releasing or synthesizing ACTIVE', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'waitingForRoster',
      selectedToken: tokenA,
      roster: [
        { ordinal: 1, present: true, local: true },
        { ordinal: 2, present: false, local: false },
      ],
    });
    await page.press('body', 'F2');
    await expect(page.locator('.app-root-popups')).toBeHidden();
    await page.press('body', 'F2');
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Players present: 1 / 2',
    );
    await expect
      .poll(() => page.evaluate(() => (globalThis as any).__resumeActions))
      .toEqual([['refresh']]);

    await project(page, {
      ...saveReady,
      phase: 'active',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Campaign resumed',
    );
    await expect(page.locator('app-campaign-resume')).toHaveCount(1);
    await project(page, {
      ...regularReady,
      phase: 'unavailable',
      resumeRequired: false,
      loadedSaveValid: false,
      selectedToken: '',
      candidates: [],
    });
    await expect(page.locator('app-campaign-resume')).toHaveCount(0);
  });

  test('authoritative ResumeRequired completion closes terminally and ordinary Resume opens only on explicit action', async ({
    page,
  }) => {
    await showMandatory(page, {
      ...saveReady,
      phase: 'synchronizing',
      selectedToken: tokenA,
    });

    await project(page, {
      ...saveReady,
      phase: 'active',
      selectedToken: tokenA,
    });
    await expect(page.locator('app-campaign-resume')).toContainText(
      'Campaign resumed',
    );

    await project(page, {
      ...regularReady,
      phase: 'unavailable',
      candidates: [],
    });
    await expect(page.locator('app-campaign-resume')).toHaveCount(0);
    await expect(page.locator('.candidate-list button')).toHaveCount(0);

    await page.evaluate(() => {
      (globalThis as any).skyrimtogether.emit('uiSurface', 'none');
    });
    await expect(page.locator('.app-root-controls')).toHaveCount(0);
    await page.evaluate(() => {
      (globalThis as any).skyrimtogether.emit('uiSurface', 'str');
    });
    await expect(page.locator('.app-root-controls')).toBeVisible();
    await expect(page.locator('app-campaign-resume')).toHaveCount(0);

    await page.getByText('Resume campaign', { exact: true }).click();
    await project(page, {
      ...regularReady,
      candidates: [
        { token: tokenA, ordinal: 1 },
        { token: tokenB, ordinal: 2 },
      ],
    });
    await expect(page.locator('.candidate-list button')).toHaveCount(2);
  });
});
