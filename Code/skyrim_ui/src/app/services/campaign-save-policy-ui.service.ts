import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

export interface CampaignSavePolicyViewState {
  inCampaign: boolean;
}

type NativeEventBridge = {
  on(event: string, callback: (value: unknown) => void): void;
  off(event: string, callback?: (value: unknown) => void): void;
};

export function parseCampaignSavePolicyState(
  serializedState: unknown,
): CampaignSavePolicyViewState | null {
  if (typeof serializedState !== 'string' || serializedState.length > 64) {
    return null;
  }
  try {
    const value = JSON.parse(serializedState) as Record<string, unknown>;
    return typeof value === 'object' &&
      value !== null &&
      typeof value.inCampaign === 'boolean'
      ? { inCampaign: value.inCampaign }
      : null;
  } catch {
    return null;
  }
}

@Injectable({ providedIn: 'root' })
export class CampaignSavePolicyUiService implements OnDestroy {
  public readonly stateChange = new BehaviorSubject<CampaignSavePolicyViewState>(
    { inCampaign: false },
  );

  private readonly eventBridge = skyrimtogether as unknown as NativeEventBridge;
  private readonly stateHandler = (serialized: unknown): void => {
    const state = parseCampaignSavePolicyState(serialized);
    if (state) {
      this.zone.run(() => this.stateChange.next(state));
    } else {
      console.error('Invalid campaign save policy state from native client');
    }
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on('campaignSavePolicyState', this.stateHandler);
  }

  public ngOnDestroy(): void {
    this.eventBridge.off('campaignSavePolicyState', this.stateHandler);
  }
}
