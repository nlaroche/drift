import { useState, useEffect, useCallback, useRef } from 'react';
import { getSliderState, getToggleState, isInJuceWebView } from '../lib/juce-bridge';

/**
 * Hook for slider/knob parameters (continuous values)
 */
export function useSliderParam(paramId: string, defaultValue: number = 0) {
  const [value, setValue] = useState(defaultValue);
  const sliderStateRef = useRef<ReturnType<typeof getSliderState>>(null);

  useEffect(() => {
    if (!isInJuceWebView()) return;

    const state = getSliderState(paramId);
    sliderStateRef.current = state;

    if (state) {
      // Get initial value
      setValue(state.getScaledValue());

      // Listen for changes from JUCE
      const handleChange = () => {
        setValue(state.getScaledValue());
      };

      // Poll for changes (JUCE relay updates)
      const interval = setInterval(handleChange, 16);
      return () => clearInterval(interval);
    }
  }, [paramId]);

  const setParam = useCallback((newValue: number) => {
    setValue(newValue);
    if (sliderStateRef.current) {
      sliderStateRef.current.setScaledValue(newValue);
    }
  }, []);

  const setNormalized = useCallback((newValue: number) => {
    if (sliderStateRef.current) {
      sliderStateRef.current.setNormalisedValue(newValue);
      setValue(sliderStateRef.current.getScaledValue());
    } else {
      setValue(newValue);
    }
  }, []);

  const dragStart = useCallback(() => {
    sliderStateRef.current?.sliderDragStarted();
  }, []);

  const dragEnd = useCallback(() => {
    sliderStateRef.current?.sliderDragEnded();
  }, []);

  return { value, setValue: setParam, setNormalized, dragStart, dragEnd };
}

/**
 * Hook for toggle/boolean parameters
 */
export function useToggleParam(paramId: string, defaultValue: boolean = false) {
  const [value, setValue] = useState(defaultValue);
  const toggleStateRef = useRef<ReturnType<typeof getToggleState>>(null);

  useEffect(() => {
    if (!isInJuceWebView()) return;

    const state = getToggleState(paramId);
    toggleStateRef.current = state;

    if (state) {
      setValue(state.getValue());

      const handleChange = () => {
        setValue(state.getValue());
      };

      const interval = setInterval(handleChange, 16);
      return () => clearInterval(interval);
    }
  }, [paramId]);

  const toggle = useCallback(() => {
    const newValue = !value;
    setValue(newValue);
    if (toggleStateRef.current) {
      toggleStateRef.current.setValue(newValue);
    }
  }, [value]);

  const setParam = useCallback((newValue: boolean) => {
    setValue(newValue);
    if (toggleStateRef.current) {
      toggleStateRef.current.setValue(newValue);
    }
  }, []);

  return { value, toggle, setValue: setParam };
}

/**
 * Hook for choice/combo parameters (using slider relay with integer mapping)
 */
export function useChoiceParam(paramId: string, numChoices: number, defaultValue: number = 0) {
  const [value, setValue] = useState(defaultValue);
  const sliderStateRef = useRef<ReturnType<typeof getSliderState>>(null);

  useEffect(() => {
    if (!isInJuceWebView()) return;

    const state = getSliderState(paramId);
    sliderStateRef.current = state;

    if (state) {
      setValue(Math.round(state.getScaledValue()));

      const handleChange = () => {
        setValue(Math.round(state.getScaledValue()));
      };

      const interval = setInterval(handleChange, 16);
      return () => clearInterval(interval);
    }
  }, [paramId]);

  const setChoice = useCallback((choice: number) => {
    setValue(choice);
    if (sliderStateRef.current) {
      sliderStateRef.current.setScaledValue(choice);
    }
  }, []);

  return { value, setChoice };
}
