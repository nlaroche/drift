import { useState, useEffect, useCallback } from 'react';
import { isInJuceWebView, addEventListener, removeEventListener } from '../lib/juce-bridge';

export interface DriftVisualizerData {
  grainActivity: number;
  currentPitch: number;
  outputLevel: number;
  isFrozen: boolean;
}

const defaultData: DriftVisualizerData = {
  grainActivity: 0,
  currentPitch: 0,
  outputLevel: 0,
  isFrozen: false,
};

export function useVisualizerData(): DriftVisualizerData {
  const [data, setData] = useState<DriftVisualizerData>(defaultData);

  const handleVisualizerData = useCallback((eventData: any) => {
    if (eventData && typeof eventData === 'object') {
      setData({
        grainActivity: eventData.grainActivity ?? 0,
        currentPitch: eventData.currentPitch ?? 0,
        outputLevel: eventData.outputLevel ?? 0,
        isFrozen: eventData.isFrozen ?? false,
      });
    }
  }, []);

  useEffect(() => {
    if (!isInJuceWebView()) {
      // Demo mode animation
      let animationFrame: number;
      let time = 0;

      const animate = () => {
        time += 0.016;

        setData({
          grainActivity: 0.3 + Math.sin(time * 2) * 0.2 + Math.random() * 0.2,
          currentPitch: Math.sin(time * 0.5) * 12,
          outputLevel: 0.4 + Math.sin(time * 3) * 0.2 + Math.random() * 0.1,
          isFrozen: false,
        });

        animationFrame = requestAnimationFrame(animate);
      };

      animate();
      return () => cancelAnimationFrame(animationFrame);
    }

    addEventListener('visualizerData', handleVisualizerData);
    return () => removeEventListener('visualizerData', handleVisualizerData);
  }, [handleVisualizerData]);

  return data;
}
