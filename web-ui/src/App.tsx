import { useState, useEffect, useRef, useCallback } from 'react';
import { useSliderParam, useToggleParam, useChoiceParam } from './hooks/useJuceParam';
import { useVisualizerData } from './hooks/useVisualizerData';
import { ActivationScreen } from './components/ActivationScreen';
import './index.css';

const DIVISIONS = ['1/1', '1/2', '1/4', '1/8', '1/16', '1/32', '1/4T', '1/8T', '1/16T', '1/4D', '1/8D', '1/16D'];

function App() {
  const [isActivated, setIsActivated] = useState(false);

  // Show activation screen first
  if (!isActivated) {
    return <ActivationScreen onActivated={() => setIsActivated(true)} />;
  }

  return <MainApp />;
}

function MainApp() {
  // Core delay params
  const time = useSliderParam('time', 400);
  const sync = useToggleParam('sync', false);
  const division = useChoiceParam('division', 12, 2);
  const feedback = useSliderParam('feedback', 50);
  const duck = useSliderParam('duck', 30);
  const taps = useChoiceParam('taps', 4, 2);
  const spread = useSliderParam('spread', 50);
  const mix = useSliderParam('mix', 50);

  // Character params
  const grit = useSliderParam('grit', 0);
  const age = useSliderParam('age', 25);
  const diffuse = useSliderParam('diffuse', 0);

  const visualizerData = useVisualizerData();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const timeRef = useRef(0);

  const smoothedLevels = useRef([0, 0, 0, 0]);
  const smoothedDrift = useRef(0);
  const ribbonOpacities = useRef([0, 0, 0, 0]);

  const wisps = useRef<Array<{x: number, y: number, length: number, phase: number, speed: number}>>([]);
  if (wisps.current.length === 0) {
    for (let i = 0; i < 6; i++) {
      wisps.current.push({
        x: Math.random() * 2000,
        y: 0.42 + Math.random() * 0.38,
        length: 150 + Math.random() * 350,
        phase: Math.random() * Math.PI * 2,
        speed: 0.8 + Math.random() * 0.5
      });
    }
  }

  const sandGrains = useRef<Array<{x: number, y: number, size: number, drift: number, speed: number}>>([]);
  if (sandGrains.current.length === 0) {
    for (let i = 0; i < 20; i++) {
      sandGrains.current.push({
        x: Math.random() * 2000,
        y: 0.65 + Math.random() * 0.3,
        size: 1 + Math.random() * 1.5,
        drift: Math.random() * Math.PI * 2,
        speed: 0.3 + Math.random() * 0.4
      });
    }
  }

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d')!;
    let animationId: number;

    const resize = () => {
      const dpr = window.devicePixelRatio || 1;
      canvas.width = window.innerWidth * dpr;
      canvas.height = window.innerHeight * dpr;
      canvas.style.width = window.innerWidth + 'px';
      canvas.style.height = window.innerHeight + 'px';
      ctx.scale(dpr, dpr);
    };

    resize();
    window.addEventListener('resize', resize);

    const render = () => {
      animationId = requestAnimationFrame(render);
      timeRef.current += 0.016;

      const w = window.innerWidth;
      const h = window.innerHeight;
      const t = timeRef.current;

      const attack = 0.25;
      const release = 0.04;

      const targetLevels = [
        visualizerData.tap1Level,
        visualizerData.tap2Level,
        visualizerData.tap3Level,
        visualizerData.tap4Level
      ];

      for (let i = 0; i < 4; i++) {
        const target = targetLevels[i];
        const current = smoothedLevels.current[i];
        const speed = target > current ? attack : release;
        smoothedLevels.current[i] += (target - current) * speed;
      }

      const driftTarget = visualizerData.duckEnvelope;
      smoothedDrift.current += (driftTarget - smoothedDrift.current) * 0.08;

      const driftAmt = smoothedDrift.current;
      const numTaps = taps.value;

      // Sky gradient
      const skyGrad = ctx.createLinearGradient(0, 0, 0, h);
      skyGrad.addColorStop(0, '#0f0805');
      skyGrad.addColorStop(0.2, '#1a0d06');
      skyGrad.addColorStop(0.4, '#3a1c10');
      skyGrad.addColorStop(0.6, '#5a2a18');
      skyGrad.addColorStop(0.8, '#7a3a20');
      skyGrad.addColorStop(1, '#4a2515');
      ctx.fillStyle = skyGrad;
      ctx.fillRect(0, 0, w, h);

      // Haze
      const hazeGrad = ctx.createLinearGradient(0, h * 0.4, 0, h * 0.65);
      hazeGrad.addColorStop(0, 'rgba(255, 180, 120, 0)');
      hazeGrad.addColorStop(0.5, 'rgba(255, 160, 100, 0.08)');
      hazeGrad.addColorStop(1, 'rgba(255, 140, 80, 0)');
      ctx.fillStyle = hazeGrad;
      ctx.fillRect(0, 0, w, h);

      // Sun
      const sunX = w * 0.75;
      const sunY = h * 0.28;
      const sunGlow = ctx.createRadialGradient(sunX, sunY, 0, sunX, sunY, 160);
      sunGlow.addColorStop(0, 'rgba(255, 220, 160, 0.6)');
      sunGlow.addColorStop(0.2, 'rgba(255, 190, 120, 0.35)');
      sunGlow.addColorStop(0.5, 'rgba(255, 150, 80, 0.15)');
      sunGlow.addColorStop(1, 'transparent');
      ctx.fillStyle = sunGlow;
      ctx.fillRect(0, 0, w, h);

      ctx.beginPath();
      ctx.arc(sunX, sunY, 18, 0, Math.PI * 2);
      const sunCore = ctx.createRadialGradient(sunX, sunY, 0, sunX, sunY, 18);
      sunCore.addColorStop(0, 'rgba(255, 240, 200, 0.9)');
      sunCore.addColorStop(0.7, 'rgba(255, 200, 140, 0.6)');
      sunCore.addColorStop(1, 'rgba(255, 180, 120, 0)');
      ctx.fillStyle = sunCore;
      ctx.fill();

      // Stars
      ctx.fillStyle = 'rgba(255, 220, 180, 0.25)';
      for (let i = 0; i < 15; i++) {
        const sx = (i * 137.5 + 50) % w;
        const sy = (i * 89.3 + 20) % (h * 0.35);
        ctx.beginPath();
        ctx.arc(sx, sy, 0.6 + (i % 3) * 0.3, 0, Math.PI * 2);
        ctx.fill();
      }

      // Far dunes
      ctx.fillStyle = '#2a1610';
      ctx.beginPath();
      ctx.moveTo(0, h);
      for (let x = 0; x <= w; x += 30) {
        const dune = Math.sin(x * 0.002 + 0.5) * 50 + Math.sin(x * 0.005) * 25;
        ctx.lineTo(x, h * 0.52 + dune);
      }
      ctx.lineTo(w, h);
      ctx.fill();

      // Mid dunes
      const midDuneShift = driftAmt * 10;
      ctx.fillStyle = '#3a2015';
      ctx.beginPath();
      ctx.moveTo(0, h);
      for (let x = 0; x <= w; x += 20) {
        const dune = Math.sin(x * 0.003 + 1 + t * 0.02) * 40 + Math.sin(x * 0.007 + midDuneShift * 0.1) * 20;
        ctx.lineTo(x, h * 0.60 + dune);
      }
      ctx.lineTo(w, h);
      ctx.fill();

      // Near dunes
      const nearDuneGrad = ctx.createLinearGradient(0, h * 0.68, 0, h);
      nearDuneGrad.addColorStop(0, '#4a2818');
      nearDuneGrad.addColorStop(1, '#2a1810');
      ctx.fillStyle = nearDuneGrad;
      ctx.beginPath();
      ctx.moveTo(0, h);
      for (let x = 0; x <= w; x += 15) {
        const dune = Math.sin(x * 0.004 + t * 0.04 + driftAmt * 0.5) * 35 + Math.sin(x * 0.01) * 15;
        ctx.lineTo(x, h * 0.70 + dune);
      }
      ctx.lineTo(w, h);
      ctx.fill();

      // Sand grains
      for (const grain of sandGrains.current) {
        grain.x += grain.speed;
        grain.y += Math.sin(t * 0.5 + grain.drift) * 0.0003;
        if (grain.x > w + 10) grain.x = -10;

        const grainY = grain.y * h;
        const alpha = 0.15 + Math.sin(t * 0.3 + grain.drift) * 0.05;
        ctx.beginPath();
        ctx.arc(grain.x, grainY, grain.size, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255, 200, 150, ${alpha})`;
        ctx.fill();
      }

      // Wind wisps
      for (const wisp of wisps.current) {
        wisp.x += wisp.speed;
        if (wisp.x > w + wisp.length) wisp.x = -wisp.length;

        const wispY = wisp.y * h;
        const wave = Math.sin(t * 0.3 + wisp.phase) * 6;

        const gradient = ctx.createLinearGradient(wisp.x, wispY, wisp.x + wisp.length, wispY);
        gradient.addColorStop(0, 'rgba(255, 210, 170, 0)');
        gradient.addColorStop(0.3, 'rgba(255, 210, 170, 0.06)');
        gradient.addColorStop(0.5, 'rgba(255, 200, 160, 0.09)');
        gradient.addColorStop(0.7, 'rgba(255, 210, 170, 0.06)');
        gradient.addColorStop(1, 'rgba(255, 210, 170, 0)');

        ctx.strokeStyle = gradient;
        ctx.lineWidth = 1.5;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.moveTo(wisp.x, wispY + wave);
        ctx.quadraticCurveTo(
          wisp.x + wisp.length * 0.5, wispY + wave + Math.sin(t * 0.2 + wisp.phase) * 4,
          wisp.x + wisp.length, wispY + wave + Math.sin(t * 0.25 + wisp.phase + 1) * 5
        );
        ctx.stroke();
      }

      // Echo ribbons
      const horizonY = h * 0.48;
      const ribbonBaseY = horizonY + 35;

      for (let tap = 0; tap < 4; tap++) {
        const isActive = tap < numTaps;
        const level = isActive ? smoothedLevels.current[tap] : 0;

        const targetOpacity = (isActive && level > 0.02) ? (0.35 + level * 0.45) : 0;
        ribbonOpacities.current[tap] += (targetOpacity - ribbonOpacities.current[tap]) * 0.03;

        if (ribbonOpacities.current[tap] < 0.01) continue;

        const tapOffset = tap * 25;
        const ribbonY = ribbonBaseY + tapOffset;
        const ribbonSpeed = 0.8 + tap * 0.2;
        const hue = 28 - tap * 6;
        const alpha = ribbonOpacities.current[tap];

        ctx.beginPath();
        ctx.moveTo(0, ribbonY);

        for (let x = 0; x <= w; x += 8) {
          const wave = Math.sin(x * 0.008 - t * ribbonSpeed + tap * 0.5) * (15 + level * 20);
          const drift = Math.sin(x * 0.003 + driftAmt * 2 + t * 0.3) * (10 * (tap + 1) * 0.5);
          ctx.lineTo(x, ribbonY + wave + drift);
        }

        for (let x = w; x >= 0; x -= 8) {
          const wave = Math.sin(x * 0.008 - t * ribbonSpeed + tap * 0.5 + 0.3) * (12 + level * 15);
          const drift = Math.sin(x * 0.003 + driftAmt * 2 + t * 0.3) * (8 * (tap + 1) * 0.5);
          ctx.lineTo(x, ribbonY + wave + drift + 4 + level * 8);
        }

        ctx.closePath();

        const ribbonGrad = ctx.createLinearGradient(0, ribbonY - 15, 0, ribbonY + 25);
        ribbonGrad.addColorStop(0, `hsla(${hue}, 55%, 65%, ${alpha * 0.7})`);
        ribbonGrad.addColorStop(0.5, `hsla(${hue}, 50%, 50%, ${alpha * 0.85})`);
        ribbonGrad.addColorStop(1, `hsla(${hue}, 45%, 38%, ${alpha * 0.5})`);

        ctx.fillStyle = ribbonGrad;
        ctx.fill();
      }

      // Vignette
      const vignette = ctx.createRadialGradient(w / 2, h / 2, h * 0.35, w / 2, h / 2, h);
      vignette.addColorStop(0, 'transparent');
      vignette.addColorStop(1, 'rgba(0, 0, 0, 0.45)');
      ctx.fillStyle = vignette;
      ctx.fillRect(0, 0, w, h);
    };

    render();

    return () => {
      cancelAnimationFrame(animationId);
      window.removeEventListener('resize', resize);
    };
  }, [taps.value, visualizerData]);

  return (
    <div className="app">
      <canvas ref={canvasRef} className="canvas" />

      <div className="header">
        <div className="logo">DRIFT</div>
        <div className="subtitle">WANDERING DELAY</div>
      </div>

      <div className="controls-panel">
        <TimeControl
          syncEnabled={sync.value}
          onSyncToggle={sync.toggle}
          timeValue={time.value}
          onTimeChange={time.setValue}
          onTimeDragStart={time.dragStart}
          onTimeDragEnd={time.dragEnd}
          divisionValue={division.value}
          onDivisionChange={division.setChoice}
        />

        <KnobControl
          label="FEEDBACK"
          value={feedback.value}
          onChange={feedback.setValue}
          onDragStart={feedback.dragStart}
          onDragEnd={feedback.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
        />

        <TapsControl value={taps.value} onChange={taps.setChoice} />

        <KnobControl
          label="SPREAD"
          value={spread.value}
          onChange={spread.setValue}
          onDragStart={spread.dragStart}
          onDragEnd={spread.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
        />

        <div className="divider" />

        <KnobControl
          label="GRIT"
          value={grit.value}
          onChange={grit.setValue}
          onDragStart={grit.dragStart}
          onDragEnd={grit.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
          small
        />

        <KnobControl
          label="AGE"
          value={age.value}
          onChange={age.setValue}
          onDragStart={age.dragStart}
          onDragEnd={age.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
          small
        />

        <KnobControl
          label="DIFFUSE"
          value={diffuse.value}
          onChange={diffuse.setValue}
          onDragStart={diffuse.dragStart}
          onDragEnd={diffuse.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
          small
        />

        <div className="divider" />

        <KnobControl
          label="DUCK"
          value={duck.value}
          onChange={duck.setValue}
          onDragStart={duck.dragStart}
          onDragEnd={duck.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
        />

        <KnobControl
          label="MIX"
          value={mix.value}
          onChange={mix.setValue}
          onDragStart={mix.dragStart}
          onDragEnd={mix.dragEnd}
          min={0}
          max={100}
          format={(v) => `${v.toFixed(0)}%`}
        />
      </div>
    </div>
  );
}

interface KnobControlProps {
  label: string;
  value: number;
  onChange: (v: number) => void;
  onDragStart?: () => void;
  onDragEnd?: () => void;
  min: number;
  max: number;
  format: (v: number) => string;
  small?: boolean;
}

function KnobControl({ label, value, onChange, onDragStart, onDragEnd, min, max, format, small }: KnobControlProps) {
  const isDragging = useRef(false);
  const startY = useRef(0);
  const startValue = useRef(0);

  const normalized = (value - min) / (max - min);
  const angle = -135 + normalized * 270;

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    isDragging.current = true;
    startY.current = e.clientY;
    startValue.current = value;
    onDragStart?.();

    const handleMouseMove = (e: MouseEvent) => {
      if (!isDragging.current) return;
      const deltaY = startY.current - e.clientY;
      const sensitivity = (max - min) / 200;
      let newValue = startValue.current + deltaY * sensitivity;
      newValue = Math.max(min, Math.min(max, newValue));
      onChange(newValue);
    };

    const handleMouseUp = () => {
      isDragging.current = false;
      onDragEnd?.();
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  }, [value, min, max, onChange, onDragStart, onDragEnd]);

  const handleWheel = useCallback((e: React.WheelEvent) => {
    e.preventDefault();
    const sensitivity = (max - min) / 100;
    let newValue = value - e.deltaY * sensitivity * 0.1;
    newValue = Math.max(min, Math.min(max, newValue));
    onChange(newValue);
  }, [value, min, max, onChange]);

  const size = small ? 38 : 50;

  return (
    <div className={`control ${small ? 'small' : ''}`}>
      <div className="control-label">{label}</div>
      <div className="knob" style={{ width: size, height: size }} onMouseDown={handleMouseDown} onWheel={handleWheel}>
        <svg viewBox="0 0 60 60" className="knob-svg">
          <circle cx="30" cy="30" r="24" fill="none" stroke="rgba(200, 150, 100, 0.12)" strokeWidth="3" strokeLinecap="round" strokeDasharray="113" strokeDashoffset="28" transform="rotate(135 30 30)" />
          <circle cx="30" cy="30" r="24" fill="none" stroke="rgba(255, 180, 120, 0.75)" strokeWidth="3" strokeLinecap="round" strokeDasharray={`${normalized * 113} 200`} strokeDashoffset="0" transform="rotate(135 30 30)" />
          <line x1="30" y1="30" x2="30" y2="12" stroke="rgba(255, 200, 150, 0.9)" strokeWidth="2" strokeLinecap="round" transform={`rotate(${angle} 30 30)`} />
          <circle cx="30" cy="30" r="3" fill="rgba(200, 150, 100, 0.15)" />
        </svg>
      </div>
      <div className="control-value">{format(value)}</div>
    </div>
  );
}

interface TapsControlProps {
  value: number;
  onChange: (v: number) => void;
}

function TapsControl({ value, onChange }: TapsControlProps) {
  return (
    <div className="control taps-control">
      <div className="control-label">TAPS</div>
      <div className="taps-buttons">
        {[1, 2, 3, 4].map((n) => (
          <button key={n} className={`tap-button ${value === n ? 'active' : ''}`} onClick={() => onChange(n)}>{n}</button>
        ))}
      </div>
    </div>
  );
}

interface TimeControlProps {
  syncEnabled: boolean;
  onSyncToggle: () => void;
  timeValue: number;
  onTimeChange: (v: number) => void;
  onTimeDragStart: () => void;
  onTimeDragEnd: () => void;
  divisionValue: number;
  onDivisionChange: (v: number) => void;
}

function TimeControl({ syncEnabled, onSyncToggle, timeValue, onTimeChange, onTimeDragStart, onTimeDragEnd, divisionValue, onDivisionChange }: TimeControlProps) {
  const isDragging = useRef(false);
  const startY = useRef(0);
  const startValue = useRef(0);

  const min = 10, max = 2000;
  const normalized = (timeValue - min) / (max - min);
  const angle = -135 + normalized * 270;

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    if (syncEnabled) return;
    isDragging.current = true;
    startY.current = e.clientY;
    startValue.current = timeValue;
    onTimeDragStart();

    const handleMouseMove = (e: MouseEvent) => {
      if (!isDragging.current) return;
      const deltaY = startY.current - e.clientY;
      const sensitivity = (max - min) / 200;
      let newValue = startValue.current + deltaY * sensitivity;
      newValue = Math.max(min, Math.min(max, newValue));
      onTimeChange(newValue);
    };

    const handleMouseUp = () => {
      isDragging.current = false;
      onTimeDragEnd();
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  }, [syncEnabled, timeValue, onTimeChange, onTimeDragStart, onTimeDragEnd]);

  const handleWheel = useCallback((e: React.WheelEvent) => {
    if (syncEnabled) return;
    e.preventDefault();
    const sensitivity = (max - min) / 100;
    let newValue = timeValue - e.deltaY * sensitivity * 0.1;
    newValue = Math.max(min, Math.min(max, newValue));
    onTimeChange(newValue);
  }, [syncEnabled, timeValue, onTimeChange]);

  return (
    <div className="control time-control">
      <div className="time-header">
        <span className="time-label">{syncEnabled ? 'SYNC' : 'TIME'}</span>
        <button className={`sync-toggle ${syncEnabled ? 'active' : ''}`} onClick={onSyncToggle} title="Toggle tempo sync">
          <svg viewBox="0 0 16 16" className="sync-icon">
            <path d="M8 3v2M8 11v2M3 8h2M11 8h2M4.5 4.5l1.4 1.4M10.1 10.1l1.4 1.4M4.5 11.5l1.4-1.4M10.1 5.9l1.4-1.4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
            <circle cx="8" cy="8" r="2" fill="currentColor"/>
          </svg>
        </button>
      </div>

      {syncEnabled ? (
        <div className="division-selector">
          <button className="div-arrow" onClick={() => onDivisionChange(Math.max(0, divisionValue - 1))}>&#9664;</button>
          <span className="division-value">{DIVISIONS[divisionValue] || '1/4'}</span>
          <button className="div-arrow" onClick={() => onDivisionChange(Math.min(11, divisionValue + 1))}>&#9654;</button>
        </div>
      ) : (
        <div className="knob" style={{ width: 50, height: 50 }} onMouseDown={handleMouseDown} onWheel={handleWheel}>
          <svg viewBox="0 0 60 60" className="knob-svg">
            <circle cx="30" cy="30" r="24" fill="none" stroke="rgba(200, 150, 100, 0.12)" strokeWidth="3" strokeLinecap="round" strokeDasharray="113" strokeDashoffset="28" transform="rotate(135 30 30)" />
            <circle cx="30" cy="30" r="24" fill="none" stroke="rgba(255, 180, 120, 0.75)" strokeWidth="3" strokeLinecap="round" strokeDasharray={`${normalized * 113} 200`} strokeDashoffset="0" transform="rotate(135 30 30)" />
            <line x1="30" y1="30" x2="30" y2="12" stroke="rgba(255, 200, 150, 0.9)" strokeWidth="2" strokeLinecap="round" transform={`rotate(${angle} 30 30)`} />
            <circle cx="30" cy="30" r="3" fill="rgba(200, 150, 100, 0.15)" />
          </svg>
        </div>
      )}

      <div className="control-value">{syncEnabled ? 'tempo' : `${timeValue.toFixed(0)}ms`}</div>
    </div>
  );
}

export default App;
