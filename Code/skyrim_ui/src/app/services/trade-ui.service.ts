import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import { TradeViewState } from '../models/trade';

type TradeBridgeArgument = string | number;
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

@Injectable({ providedIn: 'root' })
export class TradeUiService implements OnDestroy {
  public readonly stateChange = new BehaviorSubject<TradeViewState>({
    visible: false,
  });

  private readonly eventBridge = skyrimtogether as unknown as NativeEventBridge;

  private readonly stateHandler = (
    serializedState: NativeEventArgument,
  ): void => {
    if (typeof serializedState !== 'string') {
      return;
    }

    this.onTradeState(serializedState);
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on('tradeState', this.stateHandler);
  }

  public ngOnDestroy(): void {
    this.eventBridge.off('tradeState', this.stateHandler);
  }

  public acceptInvite(sessionId: string): void {
    this.sendAction('accept', sessionId);
  }

  public rejectInvite(sessionId: string): void {
    this.sendAction('reject', sessionId);
  }

  public dismissOutgoing(): void {
    this.sendAction('dismissOutgoing');
  }

  public setOffer(itemId: string, quantity: number): void {
    this.sendAction(
      'setOffer',
      itemId,
      Math.max(0, Math.trunc(quantity)),
    );
  }

  public confirm(): void {
    this.sendAction('confirm');
  }

  public cancel(): void {
    this.sendAction('cancel');
  }

  public dismiss(): void {
    this.sendAction('dismiss');
  }

  public previewItem(itemId: string): void {
    this.sendAction('preview', itemId);
  }

  public clearPreview(): void {
    this.sendAction('clearPreview');
  }

  private sendAction(
    action: string,
    ...args: TradeBridgeArgument[]
  ): void {
    try {
      const bridge = (skyrimtogether as unknown as {
        toggleDebugUI?: (
          ...bridgeArgs: TradeBridgeArgument[]
        ) => void;
      }).toggleDebugUI;

      if (typeof bridge !== 'function') {
        throw new Error('Native trade action bridge is unavailable');
      }

      bridge('__trade__', action, ...args);
    } catch (error) {
      console.error('Trade action bridge failed', error);
    }
  }

  private onTradeState(serializedState: string): void {
    try {
      const state = JSON.parse(serializedState) as TradeViewState;
      this.zone.run(() => this.stateChange.next(state));
    } catch (error) {
      console.error('Invalid trade state received from native client', error);
    }
  }
}
