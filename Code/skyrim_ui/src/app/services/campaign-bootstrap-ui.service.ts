import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

export type CampaignBootstrapScreen =
  | 'entry'
  | 'create'
  | 'join'
  | 'lobby'
  | 'error';

export interface CampaignBootstrapMember {
  name: string;
  present: boolean;
}

export interface CampaignBootstrapViewState {
  active: boolean;
  screen: CampaignBootstrapScreen;
  connected: boolean;
  busy: boolean;
  joinCode: string;
  canStart: boolean;
  members: CampaignBootstrapMember[];
  error: string;
}

type NativeEventArgument = string | number | boolean | unknown[] | null;

type NativeEventBridge = {
  on(event: string, callback: (...args: NativeEventArgument[]) => void): void;
  off(event: string, callback?: (...args: NativeEventArgument[]) => void): void;
};

const SCREENS: readonly CampaignBootstrapScreen[] = [
  'entry',
  'create',
  'join',
  'lobby',
  'error',
];

const DISPLAY_NAME_MAX_LENGTH = 24;
const CONTROL_CHARACTERS = /[\u0000-\u001f\u007f-\u009f]/u;

function isValidDisplayName(value: string): boolean {
  return (
    value === value.trim() &&
    value.length > 0 &&
    [...value].length <= DISPLAY_NAME_MAX_LENGTH &&
    !CONTROL_CHARACTERS.test(value)
  );
}

export const DEFAULT_CAMPAIGN_BOOTSTRAP_STATE: CampaignBootstrapViewState = {
  active: false,
  screen: 'entry',
  connected: false,
  busy: false,
  joinCode: '',
  canStart: false,
  members: [],
  error: '',
};

export function parseCampaignBootstrapState(
  serializedState: unknown,
): CampaignBootstrapViewState | null {
  if (typeof serializedState !== 'string' || serializedState.length > 2048) {
    return null;
  }

  try {
    const value = JSON.parse(serializedState) as Record<string, unknown>;
    if (
      typeof value !== 'object' ||
      value === null ||
      typeof value.active !== 'boolean' ||
      typeof value.screen !== 'string' ||
      !SCREENS.includes(value.screen as CampaignBootstrapScreen) ||
      typeof value.connected !== 'boolean' ||
      typeof value.busy !== 'boolean' ||
      typeof value.joinCode !== 'string' ||
      (value.joinCode !== '' &&
        !/^[ABCDEFGHJKLMNPQRSTUVWXYZ23456789]{4}$/.test(value.joinCode)) ||
      typeof value.canStart !== 'boolean' ||
      !Array.isArray(value.members) ||
      value.members.length > 10 ||
      typeof value.error !== 'string' ||
      value.error.length > 64
    ) {
      return null;
    }

    const members: CampaignBootstrapMember[] = [];
    for (const member of value.members) {
      if (
        typeof member !== 'object' ||
        member === null ||
        typeof member.name !== 'string' ||
        !isValidDisplayName(member.name) ||
        typeof member.present !== 'boolean'
      ) {
        return null;
      }
      members.push({ name: member.name, present: member.present });
    }

    return {
      active: value.active,
      screen: value.screen as CampaignBootstrapScreen,
      connected: value.connected,
      busy: value.busy,
      joinCode: value.joinCode,
      canStart: value.canStart,
      members,
      error: value.error,
    };
  } catch {
    return null;
  }
}

@Injectable({ providedIn: 'root' })
export class CampaignBootstrapUiService implements OnDestroy {
  public readonly stateChange = new BehaviorSubject<CampaignBootstrapViewState>(
    DEFAULT_CAMPAIGN_BOOTSTRAP_STATE,
  );

  private readonly eventBridge = skyrimtogether as unknown as NativeEventBridge;

  private readonly stateHandler = (serialized: NativeEventArgument): void => {
    const state = parseCampaignBootstrapState(serialized);
    if (state) {
      this.zone.run(() => this.stateChange.next(state));
    } else {
      console.error('Invalid campaign bootstrap state from native client');
    }
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on('campaignBootstrapState', this.stateHandler);
  }

  public ngOnDestroy(): void {
    this.eventBridge.off('campaignBootstrapState', this.stateHandler);
  }

  public solo(): void {
    this.send('solo');
  }

  public showCreate(): void {
    this.send('showCreate');
  }

  public showJoin(): void {
    this.send('showJoin');
  }

  public create(address: string, password: string, pseudo: string): void {
    this.send('create', address, password, '', pseudo);
  }

  public join(
    address: string,
    password: string,
    code: string,
    pseudo: string,
  ): void {
    this.send('join', address, password, code, pseudo);
  }

  public start(): void {
    this.send('start');
  }

  public back(): void {
    this.send('back');
  }

  private send(
    action: string,
    address = '',
    password = '',
    code = '',
    pseudo = '',
  ): void {
    try {
      console.info('[STRE][CampaignBootstrapBridge] dispatch', action);
      skyrimtogether.campaignBootstrapAction(
        action,
        address,
        password,
        code,
        pseudo,
      );
    } catch (error) {
      console.error('Campaign bootstrap action bridge failed', error);
    }
  }
}
