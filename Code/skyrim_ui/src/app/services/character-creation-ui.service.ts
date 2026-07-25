import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import {
  CharacterClassId,
  CharacterCreationViewState,
} from '../models/character-creation';

type NativeEventArgument = string | number | boolean | unknown[] | null;

type NativeEventBridge = {
  on(
    event: string,
    callback: (...args: NativeEventArgument[]) => void,
  ): void;
  off(
    event: string,
    callback?: (...args: NativeEventArgument[]) => void,
  ): void;
};

type CharacterCreationActionBridge = {
  characterCreationAction?: (action: string, payload?: string) => void;
};

const DEFAULT_STATE: CharacterCreationViewState = {
  visible: false,
  phase: 'inactive',
  controlsLocked: false,
  raceConfirmed: false,
  raceFormId: 0,
  raceName: '',
  selectedClassId: '',
  classConfirmed: false,
  selectedLoadoutOptions: {},
  loadoutConfirmed: false,
  buildConfirmed: false,
  error: '',
};

@Injectable({ providedIn: 'root' })
export class CharacterCreationUiService implements OnDestroy {
  public readonly stateChange =
    new BehaviorSubject<CharacterCreationViewState>(DEFAULT_STATE);

  private readonly eventBridge =
    skyrimtogether as unknown as NativeEventBridge;

  private readonly stateHandler = (
    serializedState: NativeEventArgument,
  ): void => {
    if (typeof serializedState !== 'string') {
      return;
    }

    try {
      const parsed = JSON.parse(
        serializedState,
      ) as Partial<CharacterCreationViewState>;

      const state: CharacterCreationViewState = {
        ...DEFAULT_STATE,
        ...parsed,
        selectedLoadoutOptions: {
          ...DEFAULT_STATE.selectedLoadoutOptions,
          ...(parsed.selectedLoadoutOptions ?? {}),
        },
      };

      this.zone.run(() => this.stateChange.next(state));
    } catch (error) {
      console.error(
        'Invalid character creation state received from native client',
        error,
      );
    }
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on(
      'characterCreationState',
      this.stateHandler,
    );
  }

  public ngOnDestroy(): void {
    this.eventBridge.off(
      'characterCreationState',
      this.stateHandler,
    );
  }

  public modifyRace(): void {
    this.sendAction('modifyRace');
  }

  public confirmRace(): void {
    this.sendAction('confirmRace');
  }

  public selectClass(classId: CharacterClassId): void {
    this.sendAction('selectClass', classId);
  }

  public confirmClass(): void {
    this.sendAction('confirmClass');
  }

  public reopenClassSelection(): void {
    this.sendAction('reopenClassSelection');
  }

  public selectLoadoutOption(groupId: string, optionId: string): void {
    this.sendAction('selectLoadoutOption', `${groupId}|${optionId}`);
  }

  public confirmLoadout(): void {
    this.sendAction('confirmLoadout');
  }

  public reopenLoadoutSelection(): void {
    this.sendAction('reopenLoadoutSelection');
  }

  public confirmBuild(): void {
    this.sendAction('confirmBuild');
  }

  public previewLoadoutItem(previewKey: string): void {
    this.sendAction('previewLoadoutItem', previewKey);
  }

  public clearLoadoutPreview(): void {
    this.sendAction('clearLoadoutPreview');
  }

  public setLoadoutPreviewRegion(
    left: number,
    top: number,
    width: number,
    height: number,
  ): void {
    const payload = [left, top, width, height]
      .map(value => Number.isFinite(value) ? value.toFixed(6) : '0')
      .join(',');
    this.sendAction('loadoutPreviewRegion', payload);
  }

  public retryRaceMenu(): void {
    this.sendAction('retryRaceMenu');
  }

  public recoverControls(): void {
    this.sendAction('recoverControls');
  }

  private sendAction(action: string, payload = ''): void {
    try {
      const bridge = (
        skyrimtogether as unknown as CharacterCreationActionBridge
      ).characterCreationAction;

      if (typeof bridge !== 'function') {
        throw new Error(
          'Native character creation bridge is unavailable',
        );
      }

      bridge(action, payload);
    } catch (error) {
      console.error(
        'Character creation action bridge failed',
        error,
      );
    }
  }
}
