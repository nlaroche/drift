import { useSliderParam, useToggleParam } from './hooks/useJuceParam';
import { DriftVisualizer } from './components/DriftVisualizer';
import './index.css';

function App() {
  // Grain Parameters
  const grainSize = useSliderParam('grainSize', 100);
  const grainDensity = useSliderParam('grainDensity', 8);
  const grainSpread = useSliderParam('grainSpread', 25);

  // Pitch Parameters
  const pitch = useSliderParam('pitch', 0);
  const pitchScatter = useSliderParam('pitchScatter', 0);
  const shimmer = useSliderParam('shimmer', 0);

  // Time Parameters
  const stretch = useSliderParam('stretch', 1);
  const reverse = useSliderParam('reverse', 0);
  const freeze = useToggleParam('freeze', false);

  // Texture Parameters
  const blur = useSliderParam('blur', 0);
  const warmth = useSliderParam('warmth', 30);
  const sparkle = useSliderParam('sparkle', 20);

  // Output Parameters
  const feedback = useSliderParam('feedback', 30);
  const mix = useSliderParam('mix', 50);
  const output = useSliderParam('output', 0);
  const bypass = useToggleParam('bypass', false);

  return (
    <div className={`app ${bypass.value ? 'bypassed' : ''} ${freeze.value ? 'frozen' : ''}`}>
      <header className="header">
        <h1 className="title">DRIFT</h1>
        <span className="subtitle">Granular Ambience</span>
        <button
          className={`freeze-btn ${freeze.value ? 'active' : ''}`}
          onClick={freeze.toggle}
        >
          {freeze.value ? 'FROZEN' : 'FREEZE'}
        </button>
        <button
          className={`bypass-btn ${bypass.value ? 'active' : ''}`}
          onClick={bypass.toggle}
        >
          {bypass.value ? 'BYPASSED' : 'ACTIVE'}
        </button>
      </header>

      <div className="visualizer-section">
        <DriftVisualizer
          grainSize={grainSize.value}
          pitch={pitch.value}
          shimmer={shimmer.value}
          freeze={freeze.value}
        />
      </div>

      <div className="controls-grid">
        {/* Grain Section */}
        <div className="section">
          <h2 className="section-title">Grain</h2>
          <div className="controls-row">
            <Knob
              label="Size"
              value={grainSize.value}
              onChange={grainSize.setValue}
              onDragStart={grainSize.dragStart}
              onDragEnd={grainSize.dragEnd}
              min={10}
              max={500}
              unit="ms"
              decimals={0}
            />
            <Knob
              label="Density"
              value={grainDensity.value}
              onChange={grainDensity.setValue}
              onDragStart={grainDensity.dragStart}
              onDragEnd={grainDensity.dragEnd}
              min={1}
              max={32}
              decimals={0}
            />
            <Knob
              label="Spread"
              value={grainSpread.value}
              onChange={grainSpread.setValue}
              onDragStart={grainSpread.dragStart}
              onDragEnd={grainSpread.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
          </div>
        </div>

        {/* Pitch Section */}
        <div className="section">
          <h2 className="section-title">Pitch</h2>
          <div className="controls-row">
            <Knob
              label="Pitch"
              value={pitch.value}
              onChange={pitch.setValue}
              onDragStart={pitch.dragStart}
              onDragEnd={pitch.dragEnd}
              min={-24}
              max={24}
              unit="st"
              bipolar
            />
            <Knob
              label="Scatter"
              value={pitchScatter.value}
              onChange={pitchScatter.setValue}
              onDragStart={pitchScatter.dragStart}
              onDragEnd={pitchScatter.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
            <Knob
              label="Shimmer"
              value={shimmer.value}
              onChange={shimmer.setValue}
              onDragStart={shimmer.dragStart}
              onDragEnd={shimmer.dragEnd}
              min={0}
              max={100}
              unit="%"
              highlight
            />
          </div>
        </div>

        {/* Time Section */}
        <div className="section">
          <h2 className="section-title">Time</h2>
          <div className="controls-row">
            <Knob
              label="Stretch"
              value={stretch.value}
              onChange={stretch.setValue}
              onDragStart={stretch.dragStart}
              onDragEnd={stretch.dragEnd}
              min={0.25}
              max={4}
              unit="x"
              decimals={2}
            />
            <Knob
              label="Reverse"
              value={reverse.value}
              onChange={reverse.setValue}
              onDragStart={reverse.dragStart}
              onDragEnd={reverse.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
          </div>
        </div>

        {/* Texture Section */}
        <div className="section">
          <h2 className="section-title">Texture</h2>
          <div className="controls-row">
            <Knob
              label="Blur"
              value={blur.value}
              onChange={blur.setValue}
              onDragStart={blur.dragStart}
              onDragEnd={blur.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
            <Knob
              label="Warmth"
              value={warmth.value}
              onChange={warmth.setValue}
              onDragStart={warmth.dragStart}
              onDragEnd={warmth.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
            <Knob
              label="Sparkle"
              value={sparkle.value}
              onChange={sparkle.setValue}
              onDragStart={sparkle.dragStart}
              onDragEnd={sparkle.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
          </div>
        </div>

        {/* Output Section */}
        <div className="section output-section">
          <h2 className="section-title">Output</h2>
          <div className="controls-row">
            <Knob
              label="Feedback"
              value={feedback.value}
              onChange={feedback.setValue}
              onDragStart={feedback.dragStart}
              onDragEnd={feedback.dragEnd}
              min={0}
              max={100}
              unit="%"
            />
            <Knob
              label="Mix"
              value={mix.value}
              onChange={mix.setValue}
              onDragStart={mix.dragStart}
              onDragEnd={mix.dragEnd}
              min={0}
              max={100}
              unit="%"
              highlight
            />
            <Knob
              label="Output"
              value={output.value}
              onChange={output.setValue}
              onDragStart={output.dragStart}
              onDragEnd={output.dragEnd}
              min={-24}
              max={12}
              unit="dB"
              bipolar
            />
          </div>
        </div>
      </div>
    </div>
  );
}

// Knob Component
interface KnobProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
  onDragStart?: () => void;
  onDragEnd?: () => void;
  min: number;
  max: number;
  unit?: string;
  decimals?: number;
  highlight?: boolean;
  bipolar?: boolean;
}

function Knob({ label, value, onChange, onDragStart, onDragEnd, min, max, unit = '', decimals = 1, highlight, bipolar }: KnobProps) {
  const normalized = (value - min) / (max - min);
  const angle = -135 + normalized * 270;

  const handleMouseDown = (e: React.MouseEvent) => {
    e.preventDefault();
    onDragStart?.();

    const startY = e.clientY;
    const startValue = value;
    const range = max - min;

    const handleMouseMove = (e: MouseEvent) => {
      const delta = (startY - e.clientY) / 150;
      const newValue = Math.max(min, Math.min(max, startValue + delta * range));
      onChange(newValue);
    };

    const handleMouseUp = () => {
      onDragEnd?.();
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  };

  return (
    <div className={`knob-container ${highlight ? 'highlight' : ''}`}>
      <div className="knob" onMouseDown={handleMouseDown}>
        <svg viewBox="0 0 100 100" className="knob-svg">
          {/* Track background */}
          <circle
            cx="50"
            cy="50"
            r="40"
            fill="none"
            stroke="#333"
            strokeWidth="4"
            strokeDasharray="188.5 251.3"
            strokeDashoffset="-31.4"
            strokeLinecap="round"
          />
          {/* Active track */}
          {bipolar ? (
            <circle
              cx="50"
              cy="50"
              r="40"
              fill="none"
              stroke="var(--accent-color)"
              strokeWidth="4"
              strokeDasharray={`${Math.abs(normalized - 0.5) * 188.5} 251.3`}
              strokeDashoffset={normalized >= 0.5 ? "-125.6" : `${-125.6 - (0.5 - normalized) * 188.5}`}
              strokeLinecap="round"
              className="knob-track-active"
            />
          ) : (
            <circle
              cx="50"
              cy="50"
              r="40"
              fill="none"
              stroke="var(--accent-color)"
              strokeWidth="4"
              strokeDasharray={`${normalized * 188.5} 251.3`}
              strokeDashoffset="-31.4"
              strokeLinecap="round"
              className="knob-track-active"
            />
          )}
          {/* Indicator */}
          <g transform={`rotate(${angle} 50 50)`}>
            <line
              x1="50"
              y1="20"
              x2="50"
              y2="30"
              stroke="var(--accent-color)"
              strokeWidth="3"
              strokeLinecap="round"
            />
          </g>
        </svg>
      </div>
      <div className="knob-value">
        {decimals === 0 ? Math.round(value) : value.toFixed(decimals)}{unit}
      </div>
      <div className="knob-label">{label}</div>
    </div>
  );
}

export default App;
