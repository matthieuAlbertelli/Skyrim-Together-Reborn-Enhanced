import {
  Component,
  EventEmitter,
  HostListener,
  OnDestroy,
  OnInit,
  Output,
} from '@angular/core';
import { Subscription } from 'rxjs';
import { TranslocoService } from '@ngneat/transloco';
import {
  CampaignResumeUiService,
  CampaignResumeViewState,
  DEFAULT_CAMPAIGN_RESUME_STATE,
} from '../../services/campaign-resume-ui.service';
import { Sound, SoundService } from '../../services/sound.service';
import { CampaignRosterMemberView } from '../campaign-roster/campaign-roster.component';

@Component({
  selector: 'app-campaign-resume',
  templateUrl: './campaign-resume.component.html',
  styleUrls: ['./campaign-resume.component.scss'],
})
export class CampaignResumeComponent implements OnInit, OnDestroy {
  @Output() public done = new EventEmitter<void>();
  public state: CampaignResumeViewState = DEFAULT_CAMPAIGN_RESUME_STATE;
  private readonly subscriptions = new Subscription();
  private selectionSent = false;
  private incidentActionSent = false;

  public constructor(
    private readonly resume: CampaignResumeUiService,
    private readonly sound: SoundService,
    private readonly transloco: TranslocoService,
  ) {}

  public ngOnInit(): void {
    this.subscriptions.add(
      this.resume.stateChange.subscribe(state => {
        this.state = state;
        if (state.phase === 'ready' && state.selectedToken === '') {
          this.selectionSent = false;
        }
      }),
    );
    if (!this.state.disconnectIncident && !this.state.disconnectRecovery) {
      this.resume.refresh();
    }
  }

  public ngOnDestroy(): void {
    this.subscriptions.unsubscribe();
  }

  public select(token: string): void {
    const candidate = this.state.candidates.find(
      value => value.token === token,
    );
    if (this.state.phase === 'ready' && candidate && !this.selectionSent) {
      this.selectionSent = true;
      this.resume.select(token);
      this.sound.play(Sound.Ok);
    }
  }

  public get exactCandidate() {
    if (!this.state.resumeRequired || !this.state.loadedSaveValid) {
      return undefined;
    }
    return this.state.candidates.length === 1
      ? this.state.candidates[0]
      : undefined;
  }

  public get regularCandidates() {
    return this.state.resumeRequired ? [] : this.state.candidates;
  }

  public get mandatoryRecovery(): boolean {
    return this.state.resumeRequired || this.state.disconnectRecovery;
  }

  public incidentTitleKey(): string {
    return `CAMPAIGN_RESUME.INCIDENT.${this.state.incidentKind}`;
  }

  public stayAndRecover(): void {
    if (!this.state.disconnectIncident || this.incidentActionSent) {
      return;
    }
    this.incidentActionSent = true;
    this.resume.stayAndRecover();
    this.sound.play(Sound.Ok);
  }

  public returnToMainMenu(): void {
    if (!this.state.disconnectIncident || this.incidentActionSent) {
      return;
    }
    this.incidentActionSent = true;
    console.info(
      '[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_CLICKED',
    );
    this.resume.returnToMainMenu();
    this.sound.play(Sound.Cancel);
  }

  public resumeAssociated(): void {
    const candidate = this.exactCandidate;
    if (candidate) {
      this.select(candidate.token);
    }
  }

  public progressState(step: number): 'complete' | 'current' | 'pending' {
    if (step === 0) {
      return this.state.loadedSaveValid ? 'complete' : 'current';
    }
    if (step === 1) {
      return this.state.connected ? 'complete' : 'current';
    }
    if (!this.state.connected) {
      return 'pending';
    }

    const phaseRank: Record<string, number> = {
      ready: 2,
      submitting: 2,
      admitted: 3,
      waitingForRoster: 3,
      recovery: 4,
      synchronizing: 5,
      active: 6,
      error: 2,
      unavailable: 0,
    };
    const rank = phaseRank[this.state.phase] ?? 0;
    return rank > step ? 'complete' : rank === step ? 'current' : 'pending';
  }

  public get rosterMembers(): CampaignRosterMemberView[] {
    return this.state.roster.map(member => ({
      label: member.local
        ? this.transloco.translate('CAMPAIGN_RESUME.YOU')
        : this.transloco.translate('CAMPAIGN_RESUME.PLAYER', {
            number: member.ordinal,
          }),
      present: member.present,
    }));
  }

  public get rosterSummary(): string {
    return this.transloco.translate('CAMPAIGN_RESUME.ROSTER_SUMMARY', {
      present: this.state.roster.filter(member => member.present).length,
      total: this.state.roster.length,
    });
  }

  public get rosterPresentLabel(): string {
    return this.transloco.translate('CAMPAIGN_RESUME.PRESENT');
  }

  public get rosterWaitingLabel(): string {
    return this.transloco.translate('CAMPAIGN_RESUME.WAITING');
  }

  public refresh(): void {
    if (this.state.phase === 'ready' || this.state.phase === 'error') {
      this.resume.refresh();
    }
  }

  public retry(): void {
    if (this.state.phase === 'error' && !this.state.disconnectRecovery) {
      this.resume.retry();
    }
  }

  public close(): void {
    if (this.mandatoryRecovery || this.state.disconnectIncident) {
      return;
    }
    this.sound.play(Sound.Cancel);
    this.done.next();
  }

  public errorKey(): string {
    return `CAMPAIGN_RESUME.ERROR.${this.state.error || 'UNKNOWN'}`;
  }

  @HostListener('window:keydown.escape', ['$event'])
  public onEscape(event: KeyboardEvent): void {
    if (!this.mandatoryRecovery && !this.state.disconnectIncident) {
      this.close();
    }
    event.preventDefault();
    event.stopPropagation();
  }
}
