import { Overlay } from '@angular/cdk/overlay';
import { Component, OnInit, ViewChild } from '@angular/core';
import { TranslocoService } from '@ngneat/transloco';
import { takeUntil } from 'rxjs';
import { environment } from '../../../environments/environment';
import { fadeInOutActiveAnimation } from '../../animations/fade-in-out-active.animation';
import { View } from '../../models/view.enum';
import { ClientService } from '../../services/client.service';
import { DestroyService } from '../../services/destroy.service';
import {
  SettingService,
  fontSizeToPixels,
} from '../../services/setting.service';
import { Sound, SoundService } from '../../services/sound.service';
import { UiSurfaceService } from '../../services/ui-surface.service';
import { CampaignResumeUiService } from '../../services/campaign-resume-ui.service';
import { CampaignSavePolicyUiService } from '../../services/campaign-save-policy-ui.service';
import { UiRepository } from '../../store/ui.repository';
import { ChatComponent } from '../chat/chat.component';
import { GroupComponent } from '../group/group.component';
import { controlsAnimation } from './controls.animation';
import { notificationsAnimation } from './notifications.animation';
import { map } from 'rxjs/operators';

const REVEAL_EFFECT_DURATION_MS = 10000; // todo: pass value from C++?

@Component({
  selector: 'app-root',
  templateUrl: './root.component.html',
  styleUrls: ['./root.component.scss'],
  animations: [
    controlsAnimation,
    fadeInOutActiveAnimation,
    notificationsAnimation,
  ],
  host: { 'data-app-root-game': environment.game.toString() },
  providers: [DestroyService],
})
export class RootComponent implements OnInit {
  /* ### ENUMS ### */
  readonly RootView = View;

  view$ = this.uiRepository.view$;

  connected$ = this.client.connectionStateChange.asObservable();
  menuOpen$ = this.client.openingMenuChange.asObservable();
  inGame$ = this.client.inGameStateChange.asObservable();
  active$ = this.client.activationStateChange.asObservable();
  surface$ = this.uiSurface.surfaceChange.asObservable();
  connectionInProgress$ =
    this.client.isConnectionInProgressChange.asObservable();
  revealingInProgress$ = false;
  private mandatoryCampaignViewOpen = false;

  @ViewChild('chat') private chatComp!: ChatComponent;
  @ViewChild(GroupComponent) private groupComponent: GroupComponent;

  public constructor(
    private readonly destroy$: DestroyService,
    private readonly client: ClientService,
    private readonly sound: SoundService,
    private readonly uiRepository: UiRepository,
    private readonly translocoService: TranslocoService,
    private readonly settingService: SettingService,
    private readonly uiSurface: UiSurfaceService,
    private readonly campaignResume: CampaignResumeUiService,
    public readonly campaignSavePolicy: CampaignSavePolicyUiService,
    public readonly overlay: Overlay, // used for mockup
  ) {
    this.translocoService.setActiveLang(
      this.settingService.settings.language.getValue(),
    );
  }

  public ngOnInit(): void {
    this.onInGameStateSubscription();
    this.onActivationStateSubscription();
    this.onFontSizeSubscription();
    this.campaignResume.stateChange
      .pipe(takeUntil(this.destroy$))
      .subscribe(state => {
        const mandatory =
          state.resumeRequired ||
          state.disconnectIncident ||
          state.disconnectRecovery;
        if (mandatory) {
          this.mandatoryCampaignViewOpen = true;
          this.uiRepository.openView(View.CAMPAIGN_RESUME);
        } else if (this.mandatoryCampaignViewOpen) {
          this.mandatoryCampaignViewOpen = false;
          if (this.uiRepository.getView() === View.CAMPAIGN_RESUME) {
            this.closeView();
          }
        }
      });
  }

  public onInGameStateSubscription() {
    this.client.inGameStateChange
      .pipe(takeUntil(this.destroy$))
      .subscribe(state => {
        if (!state && !this.isMandatoryCampaignView()) {
          this.closeView();
        }
      });
  }

  public onActivationStateSubscription() {
    this.client.activationStateChange
      .pipe(takeUntil(this.destroy$))
      .subscribe(state => {
        if (
          this.client.inGameStateChange.getValue() &&
          state &&
          this.uiSurface.surfaceChange.getValue() === 'str' &&
          !this.uiRepository.isViewOpen()
        ) {
          setTimeout(() => this.chatComp?.focus(), 100);
        }
        if (!state && !this.isMandatoryCampaignView()) {
          this.closeView();
        }
      });
  }

  public onFontSizeSubscription() {
    this.settingService.settings.fontSize
      .pipe(
        takeUntil(this.destroy$),
        map(size => fontSizeToPixels[size]),
      )
      .subscribe(size => {
        document.documentElement.setAttribute('style', `font-size: ${size}px;`);
      });
  }

  private isMandatoryCampaignView(): boolean {
    const state = this.campaignResume.stateChange.getValue();
    return (
      state.resumeRequired ||
      state.disconnectIncident ||
      state.disconnectRecovery
    );
  }

  public setView(view: View | null) {
    this.uiRepository.openView(view);

    if (view) {
      this.sound.play(Sound.Focus);
    } else if (this.chatComp) {
      this.chatComp.focus();
    }
  }

  public closeView() {
    this.uiRepository.openView(null);
  }

  public reconnect(): void {
    this.client.reconnect();
  }

  public revealPlayers(): void {
    if (this.revealingInProgress$) {
      return;
    }

    this.revealingInProgress$ = true;
    setTimeout(() => {
      this.revealingInProgress$ = false;
    }, REVEAL_EFFECT_DURATION_MS);

    this.sound.play(Sound.Focus);
    this.client.revealPlayers();
  }
}
