import { Injectable, NgZone, OnDestroy } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import { environment } from '../../environments/environment';

export type UiSurface = 'none' | 'str' | 'trade';

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
export class UiSurfaceService implements OnDestroy {
  public readonly surfaceChange = new BehaviorSubject<UiSurface>(
    environment.game ? 'none' : 'str',
  );

  private readonly eventBridge = skyrimtogether as unknown as NativeEventBridge;

  private readonly surfaceHandler = (
    value: NativeEventArgument,
  ): void => {
    if (value !== 'none' && value !== 'str' && value !== 'trade') {
      return;
    }

    this.zone.run(() => this.surfaceChange.next(value));
  };

  public constructor(private readonly zone: NgZone) {
    this.eventBridge.on('uiSurface', this.surfaceHandler);
  }

  public ngOnDestroy(): void {
    this.eventBridge.off('uiSurface', this.surfaceHandler);
  }
}
