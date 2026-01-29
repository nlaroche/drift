import { useState, useEffect } from 'react';
import { isInJuceWebView, addEventListener } from '../lib/juce-bridge';

export interface DriftVisualizerData {
  inputLevel: number;
  duckEnvelope: number;
  tap1Level: number;
  tap2Level: number;
  tap3Level: number;
  tap4Level: number;
}

const defaultData: DriftVisualizerData = {
  inputLevel: 0,
  duckEnvelope: 0,
  tap1Level: 0,
  tap2Level: 0,
  tap3Level: 0,
  tap4Level: 0,
};

export function useVisualizerData(): DriftVisualizerData {
  const [data, setData] = useState<DriftVisualizerData>(defaultData);

  useEffect(() => {
    if (!isInJuceWebView()) {
      // Demo animation when not in JUCE
      let animationFrame: number;
      let time = 0;

      const animate = () => {
        time += 0.016;

        // Smooth demo animation
        const pulse = Math.sin(time * 2) * 0.5 + 0.5;
        const inputLevel = 0.2 + pulse * 0.5;
        const drift = Math.sin(time * 0.3) * 0.5;

        setData({
          inputLevel: inputLevel,
          duckEnvelope: drift,
          tap1Level: inputLevel * 0.7,
          tap2Level: inputLevel * 0.5,
          tap3Level: inputLevel * 0.3,
          tap4Level: inputLevel * 0.15,
        });

        animationFrame = requestAnimationFrame(animate);
      };

      animate();
      return () => cancelAnimationFrame(animationFrame);
    }

    // In JUCE, listen for visualizer data events
    const unsubscribe = addEventListener('visualizerData', (eventData: unknown) => {
      const d = eventData as Record<string, number>;
      if (d && typeof d === 'object') {
        setData({
          inputLevel: d.inputLevel ?? 0,
          duckEnvelope: d.duckEnvelope ?? 0,
          tap1Level: d.tap1Level ?? 0,
          tap2Level: d.tap2Level ?? 0,
          tap3Level: d.tap3Level ?? 0,
          tap4Level: d.tap4Level ?? 0,
        });
      }
    });

    return unsubscribe;
  }, []);

  return data;
}
