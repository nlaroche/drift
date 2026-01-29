/**
 * React Hooks for JUCE 8 Parameter Binding
 *
 * These hooks provide easy React integration with JUCE 8's native WebView relay system.
 * They handle bidirectional sync automatically - changes from React update JUCE,
 * and changes from JUCE (automation, presets) update React.
 */

import { useState, useEffect, useCallback, useRef } from 'react';
import {
  getSliderState,
  getToggleState,
  isJuceEnvironment,
} from '../lib/juce-bridge';

// ==============================================================================
// useSliderParam - Continuous Float Parameters
// ==============================================================================

interface SliderParamReturn {
  /** Current value (0-1 normalized) */
  value: number;
  /** Set value (0-1 normalized) */
  setValue: (value: number) => void;
  /** Call when drag starts (for undo grouping) */
  dragStart: () => void;
  /** Call when drag ends (for undo grouping) */
  dragEnd: () => void;
  /** Whether running in JUCE WebView */
  isConnected: boolean;
}

/**
 * Hook for continuous float parameters (gain, mix, frequency, etc.)
 *
 * @param paramId - Must match the C++ relay identifier (e.g., "gain")
 * @param defaultValue - Default value when not running in JUCE (in scaled/native range)
 */
export function useSliderParam(
  paramId: string,
  defaultValue: number = 0.5
): SliderParamReturn {
  const [value, setValueState] = useState(defaultValue);
  const stateRef = useRef(getSliderState(paramId));
  const isConnected = isJuceEnvironment();

  useEffect(() => {
    const state = stateRef.current;

    // Sync initial value from JUCE (use scaled value for display)
    if (isConnected) {
      setValueState(state.getScaledValue());
    }

    // Listen for changes from JUCE (automation, presets, etc.)
    const listenerId = state.valueChangedEvent.addListener(() => {
      setValueState(state.getScaledValue());
    });

    return () => {
      state.valueChangedEvent.removeListener(listenerId);
    };
  }, [isConnected]);

  const setValue = useCallback((newValue: number) => {
    setValueState(newValue);
    stateRef.current.setScaledValue(newValue);
  }, []);

  const dragStart = useCallback(() => {
    stateRef.current.sliderDragStarted();
  }, []);

  const dragEnd = useCallback(() => {
    stateRef.current.sliderDragEnded();
  }, []);

  return { value, setValue, dragStart, dragEnd, isConnected };
}

// ==============================================================================
// useToggleParam - Boolean Parameters
// ==============================================================================

interface ToggleParamReturn {
  /** Current value */
  value: boolean;
  /** Set value */
  setValue: (value: boolean) => void;
  /** Toggle value */
  toggle: () => void;
  /** Whether running in JUCE WebView */
  isConnected: boolean;
}

/**
 * Hook for boolean parameters (bypass, enable, etc.)
 *
 * @param paramId - Must match the C++ relay identifier (e.g., "bypass")
 */
export function useToggleParam(
  paramId: string,
  defaultValue: boolean = false
): ToggleParamReturn {
  const [value, setValueState] = useState(defaultValue);
  const stateRef = useRef(getToggleState(paramId));
  const isConnected = isJuceEnvironment();

  useEffect(() => {
    const state = stateRef.current;

    if (isConnected) {
      setValueState(state.getValue());
    }

    const listenerId = state.valueChangedEvent.addListener(() => {
      setValueState(state.getValue());
    });

    return () => {
      state.valueChangedEvent.removeListener(listenerId);
    };
  }, [isConnected]);

  const setValue = useCallback((newValue: boolean) => {
    setValueState(newValue);
    stateRef.current.setValue(newValue);
  }, []);

  const toggle = useCallback(() => {
    const newValue = !stateRef.current.getValue();
    setValue(newValue);
  }, [setValue]);

  return { value, setValue, toggle, isConnected };
}

// ==============================================================================
// useChoiceParam - Integer Choice Parameters (using slider relay)
// ==============================================================================

interface ChoiceParamReturn {
  /** Current selected value (1-based for taps: 1, 2, 3, 4) */
  value: number;
  /** Set selected value */
  setChoice: (value: number) => void;
  /** Whether running in JUCE WebView */
  isConnected: boolean;
}

/**
 * Hook for integer choice parameters using slider relay
 * Works with AudioParameterInt that uses WebSliderRelay
 *
 * @param paramId - Must match the C++ relay identifier (e.g., "taps")
 * @param _numChoices - Number of choices (unused, for API compatibility)
 * @param defaultValue - Default value when not running in JUCE
 */
export function useChoiceParam(
  paramId: string,
  _numChoices: number,
  defaultValue: number = 1
): ChoiceParamReturn {
  const [value, setValueState] = useState(defaultValue);
  const stateRef = useRef(getSliderState(paramId));
  const isConnected = isJuceEnvironment();

  useEffect(() => {
    const state = stateRef.current;

    if (isConnected) {
      setValueState(Math.round(state.getScaledValue()));
    }

    const listenerId = state.valueChangedEvent.addListener(() => {
      setValueState(Math.round(state.getScaledValue()));
    });

    return () => {
      state.valueChangedEvent.removeListener(listenerId);
    };
  }, [isConnected]);

  const setChoice = useCallback((newValue: number) => {
    setValueState(newValue);
    stateRef.current.setScaledValue(newValue);
  }, []);

  return { value, setChoice, isConnected };
}
