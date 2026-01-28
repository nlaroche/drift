import { useRef, useEffect } from 'react';
import { useVisualizerData } from '../hooks/useVisualizerData';

interface DriftVisualizerProps {
  grainSize: number;
  pitch: number;
  shimmer: number;
  freeze: boolean;
}

interface Particle {
  x: number;
  y: number;
  vx: number;
  vy: number;
  size: number;
  alpha: number;
  hue: number;
  life: number;
}

export function DriftVisualizer({ grainSize, pitch, shimmer, freeze }: DriftVisualizerProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const particlesRef = useRef<Particle[]>([]);
  const data = useVisualizerData();

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animationFrame: number;

    const animate = () => {
      const width = canvas.width;
      const height = canvas.height;

      // Clear with fade effect
      ctx.fillStyle = freeze ? 'rgba(10, 20, 30, 0.05)' : 'rgba(13, 13, 26, 0.1)';
      ctx.fillRect(0, 0, width, height);

      // Spawn new particles based on grain activity
      const spawnRate = Math.floor(data.grainActivity * 5) + 1;
      for (let i = 0; i < spawnRate; i++) {
        if (particlesRef.current.length < 100) {
          const baseHue = 180 + (data.currentPitch / 24) * 60; // Cyan to purple based on pitch
          particlesRef.current.push({
            x: width / 2 + (Math.random() - 0.5) * 100,
            y: height / 2 + (Math.random() - 0.5) * 50,
            vx: (Math.random() - 0.5) * 2,
            vy: (Math.random() - 0.5) * 2 - 1, // Drift upward
            size: 2 + Math.random() * (grainSize / 100) * 8,
            alpha: 0.8,
            hue: baseHue + (Math.random() - 0.5) * 30,
            life: 1.0,
          });
        }
      }

      // Update and draw particles
      particlesRef.current = particlesRef.current.filter(p => {
        // Update position
        if (!freeze) {
          p.x += p.vx;
          p.y += p.vy;
          p.vy -= 0.02; // Gravity (negative = upward drift)
        }

        // Shimmer effect - add sparkle
        const sparkle = shimmer > 0 ? Math.sin(Date.now() * 0.01 + p.x) * shimmer / 200 : 0;

        // Fade out
        p.life -= freeze ? 0.002 : 0.015;
        p.alpha = p.life * (0.6 + sparkle);

        // Draw particle
        if (p.alpha > 0) {
          ctx.beginPath();
          ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);

          const gradient = ctx.createRadialGradient(p.x, p.y, 0, p.x, p.y, p.size);
          gradient.addColorStop(0, `hsla(${p.hue}, 80%, 70%, ${p.alpha})`);
          gradient.addColorStop(1, `hsla(${p.hue}, 80%, 50%, 0)`);

          ctx.fillStyle = gradient;
          ctx.fill();

          // Add glow for shimmer
          if (shimmer > 30) {
            ctx.beginPath();
            ctx.arc(p.x, p.y, p.size * 2, 0, Math.PI * 2);
            ctx.fillStyle = `hsla(${p.hue}, 90%, 80%, ${p.alpha * 0.2 * (shimmer / 100)})`;
            ctx.fill();
          }
        }

        return p.life > 0;
      });

      // Draw center glow
      const centerGlow = ctx.createRadialGradient(
        width / 2, height / 2, 0,
        width / 2, height / 2, 80
      );
      const glowIntensity = freeze ? 0.3 : data.grainActivity * 0.4;
      const glowHue = 180 + (data.currentPitch / 24) * 60;
      centerGlow.addColorStop(0, `hsla(${glowHue}, 70%, 60%, ${glowIntensity})`);
      centerGlow.addColorStop(1, 'transparent');
      ctx.fillStyle = centerGlow;
      ctx.fillRect(0, 0, width, height);

      // Draw freeze indicator
      if (freeze) {
        ctx.strokeStyle = 'rgba(100, 200, 255, 0.5)';
        ctx.lineWidth = 2;
        ctx.setLineDash([5, 5]);
        ctx.strokeRect(10, 10, width - 20, height - 20);
        ctx.setLineDash([]);

        // Frozen text
        ctx.fillStyle = 'rgba(100, 200, 255, 0.8)';
        ctx.font = '12px monospace';
        ctx.textAlign = 'center';
        ctx.fillText('FROZEN', width / 2, height - 15);
      }

      // Output level meter (subtle)
      const meterHeight = height * 0.6;
      const meterY = (height - meterHeight) / 2;
      ctx.fillStyle = 'rgba(255, 255, 255, 0.1)';
      ctx.fillRect(width - 8, meterY, 4, meterHeight);
      ctx.fillStyle = `hsla(${180 + data.outputLevel * 60}, 80%, 60%, 0.6)`;
      ctx.fillRect(width - 8, meterY + meterHeight * (1 - data.outputLevel), 4, meterHeight * data.outputLevel);

      animationFrame = requestAnimationFrame(animate);
    };

    animate();
    return () => cancelAnimationFrame(animationFrame);
  }, [data, grainSize, pitch, shimmer, freeze]);

  return (
    <div className="visualizer-container">
      <canvas
        ref={canvasRef}
        width={460}
        height={180}
        className="visualizer-canvas"
      />
    </div>
  );
}
