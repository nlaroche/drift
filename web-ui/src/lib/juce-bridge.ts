/**
 * JUCE WebView Bridge Utilities
 * Provides type-safe access to JUCE relay system
 */

declare global {
  interface Window {
    __JUCE__?: {
      backend?: {
        addEventListener: (event: string, callback: (data: any) => void) => void;
        removeEventListener: (event: string, callback: (data: any) => void) => void;
        emitEvent: (event: string, data: any) => void;
      };
      initialisationData?: any;
      getSliderState?: (id: string) => {
        getNormalisedValue: () => number;
        getScaledValue: () => number;
        setNormalisedValue: (val: number) => void;
        setScaledValue: (val: number) => void;
        sliderDragStarted: () => void;
        sliderDragEnded: () => void;
        getProperties: () => any;
      };
      getToggleState?: (id: string) => {
        getValue: () => boolean;
        setValue: (val: boolean) => void;
        getProperties: () => any;
      };
    };
  }
}

export function isInJuceWebView(): boolean {
  return typeof window.__JUCE__ !== 'undefined';
}

export function getSliderState(paramId: string) {
  if (!isInJuceWebView() || !window.__JUCE__?.getSliderState) {
    return null;
  }
  try {
    return window.__JUCE__.getSliderState(paramId);
  } catch {
    return null;
  }
}

export function getToggleState(paramId: string) {
  if (!isInJuceWebView() || !window.__JUCE__?.getToggleState) {
    return null;
  }
  try {
    return window.__JUCE__.getToggleState(paramId);
  } catch {
    return null;
  }
}

export function emitEvent(event: string, data: any = {}) {
  if (isInJuceWebView() && window.__JUCE__?.backend?.emitEvent) {
    window.__JUCE__.backend.emitEvent(event, data);
  }
}

export function addEventListener(event: string, callback: (data: any) => void) {
  if (isInJuceWebView() && window.__JUCE__?.backend?.addEventListener) {
    window.__JUCE__.backend.addEventListener(event, callback);
  }
}

export function removeEventListener(event: string, callback: (data: any) => void) {
  if (isInJuceWebView() && window.__JUCE__?.backend?.removeEventListener) {
    window.__JUCE__.backend.removeEventListener(event, callback);
  }
}
