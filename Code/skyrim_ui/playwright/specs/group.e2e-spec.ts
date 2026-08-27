import { createTest, expect } from '@ngx-playwright/test';
import type { MockPlayer } from '../../src/app/mock/mock-player.js';

import { ApplicationScreen } from '../screens/main-screen.js';


const test = createTest(ApplicationScreen);

test.describe('Group', () => {

  test.beforeEach(async ({ page }) => {
    await page.waitForSelector('.app-root-controls', { state: 'attached' });
    await page.press('body', 'F2');
    await page.waitForSelector('.app-root-controls', { state: 'visible' });

    // connect
    await page
      .locator('app-window.app-root-menu')
      .getByRole('button', { name: /^(Connect|Connexion)$/ })
      .click();
    await page.waitForSelector('//app-connect');
    await page.locator('//app-connect/div[1]/input[1]').fill('test');
    await page.locator('//app-connect/div[1]/input[2]').fill('test');
    await page.click('//app-connect/div[1]/app-action-buttons[1]/button[1]');
    await expect(
      page
        .locator('app-window.app-root-menu')
        .getByRole('button', { name: /^(Disconnect|Déconnexion)$/ }),
    ).toBeVisible();
  });

  test('Invite & Kick', async ({ page }) => {
    const player = await page.evaluate('skyrimtogether.addMockPlayer()') as MockPlayer;

    // open player manager
    await page
      .locator('app-window.app-root-menu')
      .getByRole('button', {
        name: /^(Player Manager|Gestion des joueurs)$/,
      })
      .click();
    const playerManager = page.locator('app-player-manager');
    await playerManager
      .getByRole('button', { name: /^(Party Menu|Gestion du groupe)$/ })
      .click();

    // launch party
    await expect(page.locator('.player-list-table')).toHaveCount(0);
    await page
      .locator('app-party-menu')
      .getByRole('button', { name: /^(Launch party|Créer le groupe)$/ })
      .click();
    await expect(page.locator('.player-list-table')).toHaveCount(0);
    await expect(page.locator('app-party-menu .no-players')).toHaveText(
      /^(There is nobody in your party yet|Il n'y a encore personne dans votre groupe)/,
    );

    // open playerlist and invite player
    await playerManager
      .getByRole('button', { name: /^(Player List|Liste des joueurs)$/ })
      .click();
    const playerRow = page
      .locator('app-player-list tbody tr')
      .filter({ hasText: player.name });
    const inviteButton = playerRow.getByRole('button', {
      name: /^(Invite|Inviter)$/,
    });
    await inviteButton.click();
    await expect(inviteButton).toBeDisabled();

    // accept invite
    await page.evaluate(`skyrimtogether.accteptMockPlayerInvite(${ player.id })`);
    await expect(inviteButton).toBeDisabled();

    // check member list
    await playerManager
      .getByRole('button', { name: /^(Party Menu|Gestion du groupe)$/ })
      .click();
    const memberRows = page.locator('app-party-menu .member-list tbody tr');
    const memberRow = memberRows.filter({ hasText: player.name });
    await expect(memberRows).toHaveCount(1);
    await expect(memberRow.getByRole('cell', { name: `${ player.level }`, exact: true })).toBeVisible();
    await expect(memberRow.getByRole('cell', { name: player.name, exact: true })).toBeVisible();
    await expect(memberRow.getByRole('cell', { name: player.cellName, exact: true })).toBeVisible();

    // kick member
    await memberRow
      .getByRole('button', { name: /^(Kick|Exclure)$/ })
      .click();
    await expect(page.locator('.player-list-table')).toHaveCount(0);

    // check invite button
    await playerManager
      .getByRole('button', { name: /^(Player List|Liste des joueurs)$/ })
      .click();
    await expect(
      page
        .locator('app-player-list tbody tr')
        .filter({ hasText: player.name })
        .getByRole('button', { name: /^(Invite|Inviter)$/ }),
    ).toBeEnabled();
  });

  test('Launch & Leave Party', async ({ page }) => {
    // open player manager
    await page
      .locator('app-window.app-root-menu')
      .getByRole('button', {
        name: /^(Player Manager|Gestion des joueurs)$/,
      })
      .click();
    const playerManager = page.locator('app-player-manager');
    await playerManager
      .getByRole('button', { name: /^(Party Menu|Gestion du groupe)$/ })
      .click();

    // launch party
    const partyMenu = page.locator('app-party-menu');
    const launchParty = partyMenu.getByRole('button', {
      name: /^(Launch party|Créer le groupe)$/,
    });
    await expect(launchParty).toBeVisible();
    await launchParty.click();

    // leave party
    const leaveParty = partyMenu.getByRole('button', {
      name: /^(Leave party|Quitter le groupe)$/,
    });
    await expect(leaveParty).toBeVisible();
    await leaveParty.click();

    // check if the launch button reappeared
    await expect(launchParty).toBeVisible();

    // close the player manager
    await playerManager
      .getByRole('button', { name: /^(Back|Retour)$/ })
      .click();
    await page.waitForSelector('//app-player-manager', { state: 'detached' });
  });
});
