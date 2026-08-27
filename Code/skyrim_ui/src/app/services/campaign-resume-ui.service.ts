import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

export type CampaignResumePhase =
  | 'unavailable'
  | 'ready'
  | 'submitting'
  | 'admitted'
  | 'waitingForRoster'
  | 'recovery'
  | 'synchronizing'
  | 'active'
  | 'error';

export type CampaignRecoveryIncidentKind =
  | 'remotePlayerMissing'
  | 'multiplePlayersMissing'
  | 'localTransportLost'
  | 'rosterRestored';

export interface CampaignResumeCandidateView {
  token: string;
  ordinal: number;
}

export interface CampaignResumeRosterMemberView {
  ordinal: number;
  present: boolean;
  local: boolean;
}

export interface CampaignResumeViewState {
  phase: CampaignResumePhase;
  connected: boolean;
  resumeRequired: boolean;
  disconnectIncident: boolean;
  disconnectRecovery: boolean;
  incidentKind: CampaignRecoveryIncidentKind;
  missingMembers: number;
  loadedSaveValid: boolean;
  selectedToken: string;
  candidates: CampaignResumeCandidateView[];
  roster: CampaignResumeRosterMemberView[];
  error: string;
}

type NativeEventArgument = string | number | boolean | unknown[] | null;
type NativeEventBridge = {
  on(event: string, callback: (...args: NativeEventArgument[]) => void): void;
  off(event: string, callback?: (...args: NativeEventArgument[]) => void): void;
};

const PHASES: readonly CampaignResumePhase[] = [
  'unavailable',
  'ready',
  'submitting',
  'admitted',
  'waitingForRoster',
  'recovery',
  'synchronizing',
  'active',
  'error',
];
const TOKEN_PATTERN = /^[0-9a-f]{32}$/;
const INCIDENT_KINDS: readonly CampaignRecoveryIncidentKind[] = [
  'remotePlayerMissing',
  'multiplePlayersMissing',
  'localTransportLost',
  'rosterRestored',
];

export const DEFAULT_CAMPAIGN_RESUME_STATE: CampaignResumeViewState = {
  phase: 'unavailable',
  connected: false,
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

export function parseCampaignResumeState(
  serializedState: unknown,
): CampaignResumeViewState | null {
  if (typeof serializedState !== 'string' || serializedState.length > 32768) {
    return null;
  }
  try {
    const value = JSON.parse(serializedState) as Record<string, unknown>;
    if (
      typeof value !== 'object' ||
      value === null ||
      typeof value.phase !== 'string' ||
      !PHASES.includes(value.phase as CampaignResumePhase) ||
      typeof value.connected !== 'boolean' ||
      typeof value.resumeRequired !== 'boolean' ||
      typeof value.disconnectIncident !== 'boolean' ||
      typeof value.disconnectRecovery !== 'boolean' ||
      (value.disconnectIncident && value.disconnectRecovery) ||
      (value.resumeRequired &&
        (value.disconnectIncident || value.disconnectRecovery)) ||
      typeof value.incidentKind !== 'string' ||
      !INCIDENT_KINDS.includes(
        value.incidentKind as CampaignRecoveryIncidentKind,
      ) ||
      typeof value.missingMembers !== 'number' ||
      !Number.isInteger(value.missingMembers) ||
      value.missingMembers < 0 ||
      value.missingMembers > 9 ||
      typeof value.loadedSaveValid !== 'boolean' ||
      typeof value.selectedToken !== 'string' ||
      (value.selectedToken !== '' &&
        !TOKEN_PATTERN.test(value.selectedToken)) ||
      !Array.isArray(value.candidates) ||
      value.candidates.length > 256 ||
      !Array.isArray(value.roster) ||
      value.roster.length > 10 ||
      typeof value.error !== 'string' ||
      value.error.length > 64
    ) {
      return null;
    }

    const tokens = new Set<string>();
    const candidates: CampaignResumeCandidateView[] = [];
    for (const candidate of value.candidates) {
      if (
        typeof candidate !== 'object' ||
        candidate === null ||
        !('token' in candidate) ||
        !('ordinal' in candidate) ||
        typeof candidate.token !== 'string' ||
        !TOKEN_PATTERN.test(candidate.token) ||
        tokens.has(candidate.token) ||
        typeof candidate.ordinal !== 'number' ||
        !Number.isInteger(candidate.ordinal) ||
        candidate.ordinal < 1 ||
        candidate.ordinal > 256
      ) {
        return null;
      }
      tokens.add(candidate.token);
      candidates.push({
        token: candidate.token,
        ordinal: candidate.ordinal,
      });
    }
    if (value.selectedToken !== '' && !tokens.has(value.selectedToken)) {
      return null;
    }
    const roster: CampaignResumeRosterMemberView[] = [];
    let localMembers = 0;
    for (let index = 0; index < value.roster.length; ++index) {
      const member = value.roster[index];
      if (
        typeof member !== 'object' ||
        member === null ||
        !('ordinal' in member) ||
        !('present' in member) ||
        !('local' in member) ||
        member.ordinal !== index + 1 ||
        typeof member.present !== 'boolean' ||
        typeof member.local !== 'boolean'
      ) {
        return null;
      }
      if (member.local) {
        ++localMembers;
      }
      roster.push({
        ordinal: member.ordinal,
        present: member.present,
        local: member.local,
      });
    }
    if (localMembers > 1) {
      return null;
    }
    if (value.disconnectIncident || value.disconnectRecovery) {
      if (localMembers !== 1) {
        return null;
      }
      const missingRemoteMembers = roster.filter(
        member => !member.local && !member.present,
      ).length;
      if (missingRemoteMembers !== value.missingMembers) {
        return null;
      }
    }
    return {
      phase: value.phase as CampaignResumePhase,
      connected: value.connected,
      resumeRequired: value.resumeRequired,
      disconnectIncident: value.disconnectIncident,
      disconnectRecovery: value.disconnectRecovery,
      incidentKind: value.incidentKind as CampaignRecoveryIncidentKind,
      missingMembers: value.missingMembers,
      loadedSaveValid: value.loadedSaveValid,
      selectedToken: value.selectedToken,
      candidates,
      roster,
      error: value.error,
    };
  } catch {
    return null;
  }
}

@Injectable({ providedIn: 'root' })
export class CampaignResumeUiService implements OnDestroy {
  public readonly stateChange = new BehaviorSubject<CampaignResumeViewState>(
    DEFAULT_CAMPAIGN_RESUME_STATE,
  );

  private readonly eventBridge = skyrimtogether as unknown as NativeEventBridge;
  private readonly stateHandler = (serialized: NativeEventArgument): void => {
    const state = parseCampaignResumeState(serialized);
    if (state) {
      this.zone.run(() => this.stateChange.next(state));
    } else {
      console.error('Invalid campaign resume state from native client');
    }
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on('campaignResumeState', this.stateHandler);
  }

  public ngOnDestroy(): void {
    this.eventBridge.off('campaignResumeState', this.stateHandler);
  }

  public refresh(): void {
    skyrimtogether.campaignResumeAction('refresh');
  }

  public retry(): void {
    skyrimtogether.campaignResumeAction('retry');
  }

  public select(token: string): void {
    if (TOKEN_PATTERN.test(token)) {
      skyrimtogether.campaignResumeAction('select', token);
    }
  }

  public stayAndRecover(): void {
    skyrimtogether.campaignResumeAction('stayAndRecover');
  }

  public returnToMainMenu(): void {
    console.info(
      '[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_BRIDGE_SEND',
    );
    skyrimtogether.campaignResumeAction('returnToMainMenu');
  }
}
