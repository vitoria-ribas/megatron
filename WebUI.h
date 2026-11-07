#pragma once

#include <Arduino.h>

// ==============================================================================
// MEGATRON // TACTICAL COMMAND HUD (CYBERPUNK SCI-FI INTERFACE)
// 100% 7-Bit ASCII & HTML Entities - Imune a problemas de encoding / Mojibake
// ==============================================================================
const char PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>MEGATRON // TACTICAL COMMAND HUD</title>
<link rel="icon" href="data:,">
<style>
  :root {
    --bg-space: #050811;
    --panel-bg: rgba(10, 16, 30, 0.88);
    --panel-border: rgba(0, 242, 254, 0.28);
    --panel-border-bright: #00f2fe;
    --cyan: #00f2fe;
    --cyan-glow: rgba(0, 242, 254, 0.45);
    --green: #10b981;
    --green-glow: rgba(16, 185, 129, 0.45);
    --amber: #f59e0b;
    --amber-glow: rgba(245, 158, 11, 0.45);
    --red: #ef4444;
    --red-glow: rgba(239, 68, 68, 0.55);
    --text-primary: #f8fafc;
    --text-muted: #94a3b8;
    --text-dim: #475569;
    --font-orbitron: "Orbitron", -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    --font-mono: "JetBrains Mono", "Courier New", monospace;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-tap-highlight-color: transparent; }

  body {
    background: var(--bg-space);
    background-image: 
      radial-gradient(circle at 50% 0%, rgba(0, 242, 254, 0.08) 0%, transparent 60%),
      linear-gradient(rgba(0, 242, 254, 0.02) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0, 242, 254, 0.02) 1px, transparent 1px);
    background-size: 100% 100%, 30px 30px, 30px 30px;
    color: var(--text-primary);
    font-family: var(--font-orbitron);
    min-height: 100vh;
    padding: 10px;
    display: flex;
    flex-direction: column;
    align-items: center;
    overflow-x: hidden;
  }

  body::before {
    content: " ";
    position: fixed;
    top: 0; left: 0; bottom: 0; right: 0;
    background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%);
    background-size: 100% 4px;
    pointer-events: none;
    z-index: 100;
    opacity: 0.5;
  }

  .hud-container {
    width: 100%;
    max-width: 1400px;
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .tech-chamfer {
    clip-path: polygon(0 0, calc(100% - 10px) 0, 100% 10px, 100% 100%, 10px 100%, 0 calc(100% - 10px));
  }
  .tech-chamfer-lg {
    clip-path: polygon(0 0, calc(100% - 18px) 0, 100% 18px, 100% 100%, 18px 100%, 0 calc(100% - 18px));
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: var(--panel-bg);
    border: 1px solid var(--panel-border);
    padding: 10px 18px;
    backdrop-filter: blur(10px);
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.6), inset 0 0 15px rgba(0, 242, 254, 0.05);
  }

  .brand-block {
    display: flex;
    align-items: center;
    gap: 12px;
  }
  .brand-icon {
    width: 36px;
    height: 36px;
    background: linear-gradient(135deg, #0284c7, #00f2fe);
    border-radius: 6px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 18px;
    color: #fff;
    box-shadow: 0 0 15px var(--cyan-glow);
  }
  .brand-titles h1 {
    font-size: 1.15rem;
    font-weight: 900;
    letter-spacing: 2px;
    color: #fff;
    text-shadow: 0 0 10px var(--cyan-glow);
  }
  .brand-titles p {
    font-family: var(--font-mono);
    font-size: 0.65rem;
    color: var(--cyan);
    letter-spacing: 1px;
    text-transform: uppercase;
  }

  .telemetry-status-strip {
    display: flex;
    align-items: center;
    gap: 16px;
    flex-wrap: wrap;
  }

  .status-badge {
    display: flex;
    align-items: center;
    gap: 7px;
    padding: 6px 14px;
    font-size: 0.75rem;
    font-weight: 800;
    letter-spacing: 1px;
    text-transform: uppercase;
    background: rgba(16, 185, 129, 0.12);
    border: 1px solid rgba(16, 185, 129, 0.4);
    color: var(--green);
    cursor: pointer;
  }
  .status-badge.disconnected {
    background: rgba(239, 68, 68, 0.12);
    border-color: rgba(239, 68, 68, 0.4);
    color: var(--red);
  }
  .pulse-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: currentColor;
    box-shadow: 0 0 8px currentColor;
    animation: livePulse 1.8s infinite;
  }
  @keyframes livePulse {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.4; transform: scale(1.3); }
  }

  .battery-gauge {
    display: flex;
    align-items: center;
    gap: 8px;
    background: rgba(15, 23, 42, 0.6);
    border: 1px solid var(--panel-border);
    padding: 5px 12px;
  }
  .bat-icon {
    width: 22px;
    height: 12px;
    border: 1.5px solid var(--cyan);
    border-radius: 2px;
    padding: 1px;
    position: relative;
    display: flex;
  }
  .bat-icon::after {
    content: '';
    position: absolute;
    right: -4px;
    top: 2px;
    width: 2px;
    height: 5px;
    background: var(--cyan);
    border-radius: 0 1px 1px 0;
  }
  .bat-fill {
    height: 100%;
    width: 85%;
    background: var(--green);
    transition: width 0.3s;
  }
  .bat-text {
    font-family: var(--font-mono);
    font-size: 0.75rem;
    color: var(--text-primary);
  }

  .dashboard-grid {
    display: grid;
    grid-template-columns: 320px 1fr 340px;
    gap: 10px;
    align-items: start;
    width: 100%;
  }

  @media (max-width: 960px) {
    body {
      padding: 6px;
    }
    header {
      padding: 8px 12px;
      flex-wrap: wrap;
      gap: 8px;
    }
    .brand-titles h1 {
      font-size: 0.95rem;
    }
    .brand-titles p {
      font-size: 0.55rem;
    }
    .telemetry-status-strip {
      gap: 8px;
    }
    .status-badge, .battery-gauge {
      padding: 4px 8px;
      font-size: 0.68rem;
    }
    .dashboard-grid {
      display: flex;
      flex-direction: column;
      gap: 8px;
      width: 100%;
    }
    .col-center {
      order: 1; /* HERO RADAR NO TOPO NO CELULAR */
      width: 100%;
    }
    .col-right {
      order: 2; /* CONTROLES E D-PAD IMEDIATAMENTE ABAIXO DO RADAR */
      width: 100%;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .col-right .card-dpad {
      order: 1; /* D-PAD NO TOPO DA COLUNA DE CONTROLE PARA MANUSEIO IMEDIATO */
    }
    .col-right .card-modes {
      order: 2;
    }
    .col-right .card-speed {
      order: 3;
    }
    .col-right .card-logs {
      order: 4;
    }
    .col-left {
      order: 3; /* TELEMETRIA SECUNDARIA ABAIXO DOS CONTROLES */
      width: 100%;
    }
    .radar-card {
      width: 100%;
      max-width: 100%;
      margin: 0 auto;
    }
    canvas#radarCanvas {
      width: 100%;
      max-width: min(100%, 40vh);
      height: auto;
      aspect-ratio: 1 / 1;
      margin: 0 auto;
    }
    .dpad-container {
      grid-template-rows: repeat(3, 58px);
      grid-template-columns: repeat(3, 70px);
      gap: 8px;
      max-width: 250px;
      margin: 4px auto;
    }
    .dpad-btn {
      font-size: 1.8rem;
    }
    .dpad-center {
      font-size: 1.5rem;
    }
    .canvas-tools-overlay {
      bottom: 8px;
      right: 8px;
      gap: 4px;
    }
    .hud-tool-btn {
      width: 30px;
      height: 30px;
      font-size: 13px;
    }
    .waypoint-prompt-badge {
      bottom: 8px;
      left: 8px;
      font-size: 0.65rem;
      padding: 3px 8px;
    }
  }

  .hud-card {
    background: var(--panel-bg);
    border: 1px solid var(--panel-border);
    padding: 12px;
    backdrop-filter: blur(10px);
    display: flex;
    flex-direction: column;
    gap: 8px;
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.4);
    position: relative;
  }
  .hud-card::before {
    content: '';
    position: absolute;
    top: 0; left: 0;
    width: 8px; height: 8px;
    border-top: 2px solid var(--cyan);
    border-left: 2px solid var(--cyan);
  }
  .hud-card::after {
    content: '';
    position: absolute;
    bottom: 0; right: 0;
    width: 8px; height: 8px;
    border-bottom: 2px solid var(--cyan);
    border-right: 2px solid var(--cyan);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid rgba(0, 242, 254, 0.15);
    padding-bottom: 6px;
  }
  .card-title {
    font-size: 0.72rem;
    font-weight: 800;
    letter-spacing: 1.5px;
    color: var(--cyan);
    text-transform: uppercase;
  }
  .card-tag {
    font-family: var(--font-mono);
    font-size: 0.6rem;
    color: var(--text-dim);
  }

  .telemetry-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 0;
    font-family: var(--font-mono);
    font-size: 0.78rem;
    border-bottom: 1px dashed rgba(255, 255, 255, 0.05);
  }
  .tel-label { color: var(--text-muted); }
  .tel-val { font-weight: 700; color: #fff; }
  .tel-val.neon { color: var(--cyan); text-shadow: 0 0 8px var(--cyan-glow); }

  .imu-wrapper {
    position: relative;
    width: 100%;
    height: 100px;
    background: #020617;
    border: 1px solid var(--panel-border);
    overflow: hidden;
    display: flex;
    align-items: center;
    justify-content: center;
    margin-top: 4px;
  }
  .horizon-sky {
    position: absolute;
    width: 160%;
    height: 160%;
    top: -30%;
    background: linear-gradient(180deg, rgba(2, 132, 199, 0.3) 0%, rgba(2, 132, 199, 0.05) 50%, rgba(180, 83, 9, 0.05) 50%, rgba(180, 83, 9, 0.3) 100%);
    transition: transform 0.1s linear;
  }
  .horizon-line {
    position: absolute;
    width: 100%;
    height: 2px;
    background: var(--cyan);
    box-shadow: 0 0 8px var(--cyan-glow);
  }
  .horizon-reticle {
    position: absolute;
    width: 40px;
    height: 40px;
    border: 1.5px solid rgba(255, 255, 255, 0.4);
    border-radius: 50%;
    pointer-events: none;
  }
  .horizon-reticle::before {
    content: '';
    position: absolute;
    top: 50%; left: -10px; right: -10px;
    height: 1px;
    background: var(--amber);
  }

  .model-view-box {
    position: relative;
    width: 100%;
    aspect-ratio: 1;
    max-height: 230px;
    background: #030712;
    border: 1px solid rgba(0, 242, 254, 0.25);
    display: flex;
    align-items: center;
    justify-content: center;
    overflow: hidden;
    box-shadow: inset 0 0 20px rgba(0, 242, 254, 0.08);
  }
  .model-img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    filter: drop-shadow(0 0 14px rgba(0, 242, 254, 0.45));
    transition: transform 0.3s ease, filter 0.3s ease;
  }
  .model-view-box:hover .model-img {
    transform: scale(1.04);
    filter: drop-shadow(0 0 20px rgba(0, 242, 254, 0.7));
  }
  .model-overlay-tag {
    position: absolute;
    bottom: 8px;
    right: 8px;
    font-family: var(--font-orbitron);
    font-size: 0.65rem;
    font-weight: 700;
    color: var(--cyan);
    background: rgba(3, 7, 18, 0.88);
    border: 1px solid var(--panel-border-bright);
    padding: 3px 8px;
    letter-spacing: 1px;
    box-shadow: 0 0 8px rgba(0, 242, 254, 0.2);
    pointer-events: none;
  }
  .model-scanline {
    position: absolute;
    top: 0; left: 0; right: 0; bottom: 0;
    background: linear-gradient(rgba(0, 242, 254, 0) 50%, rgba(0, 242, 254, 0.03) 50%);
    background-size: 100% 4px;
    pointer-events: none;
  }

  .proximity-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
    margin-top: 4px;
  }
  .prox-cell {
    background: rgba(15, 23, 42, 0.6);
    border: 1px solid var(--panel-border);
    padding: 6px 8px;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  .prox-label { font-family: var(--font-mono); font-size: 0.6rem; color: var(--text-dim); }
  .prox-val { font-family: var(--font-mono); font-size: 0.88rem; font-weight: 800; color: #fff; }
  .prox-bar { height: 4px; width: 100%; background: #1e293b; border-radius: 2px; overflow: hidden; margin-top: 2px; }
  .prox-fill { height: 100%; width: 50%; background: var(--green); transition: width 0.2s, background 0.2s; }

  .radar-card {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    position: relative;
    padding: 0;
    overflow: hidden;
    background: #040812;
    align-self: start;
    width: 100%;
  }
  canvas#radarCanvas {
    width: 100%;
    max-width: 560px;
    height: auto;
    aspect-ratio: 1 / 1;
    display: block;
    cursor: crosshair;
    background: #040812;
    touch-action: none;
  }

  .canvas-tools-overlay {
    position: absolute;
    bottom: 14px;
    right: 14px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    z-index: 10;
  }
  .hud-tool-btn {
    width: 34px;
    height: 34px;
    background: rgba(10, 16, 30, 0.85);
    border: 1px solid var(--panel-border);
    color: var(--cyan);
    font-size: 15px;
    font-weight: 900;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.15s;
    backdrop-filter: blur(8px);
  }
  .hud-tool-btn:hover {
    background: rgba(0, 242, 254, 0.2);
    border-color: var(--cyan);
    box-shadow: 0 0 10px var(--cyan-glow);
  }
  .hud-tool-btn:active { transform: scale(0.92); }

  .waypoint-prompt-badge {
    position: absolute;
    bottom: 12px;
    left: 12px;
    background: rgba(10, 16, 30, 0.9);
    border: 1px solid var(--panel-border-bright);
    padding: 6px 14px;
    font-family: var(--font-mono);
    font-size: 0.7rem;
    color: #fff;
    display: flex;
    align-items: center;
    gap: 8px;
    box-shadow: 0 0 15px rgba(0, 0, 0, 0.8);
    pointer-events: none;
    z-index: 10;
  }

  .mode-buttons-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
  }
  .mode-btn {
    font-family: var(--font-orbitron);
    font-size: 0.75rem;
    font-weight: 800;
    letter-spacing: 1px;
    padding: 12px 6px;
    background: rgba(15, 23, 42, 0.7);
    border: 1px solid var(--panel-border);
    color: var(--text-muted);
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    transition: all 0.15s ease;
  }
  .mode-btn:hover { background: rgba(0, 242, 254, 0.15); color: #fff; border-color: var(--cyan); }
  .mode-btn.active {
    background: linear-gradient(135deg, rgba(0, 242, 254, 0.3), rgba(2, 132, 199, 0.4));
    border-color: var(--cyan);
    color: #fff;
    box-shadow: 0 0 15px var(--cyan-glow);
  }

  .estop-btn {
    grid-column: span 2;
    color: #fff;
    font-size: 1.05rem;
    font-weight: 900;
    letter-spacing: 2px;
    padding: 14px;
    cursor: pointer;
    transition: all 0.25s ease;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    text-shadow: 0 0 8px rgba(0, 0, 0, 0.8);
  }

  /* ESTADO DESLIGADO: VERMELHO */
  .estop-btn.state-off, .estop-btn:not(.state-on) {
    background: linear-gradient(135deg, #991b1b, #ef4444);
    border: 2px solid #f87171;
    box-shadow: 0 0 20px rgba(239, 68, 68, 0.6);
    animation: estopPulseRed 2s infinite alternate;
  }
  .estop-btn.state-off:hover, .estop-btn:not(.state-on):hover {
    background: linear-gradient(135deg, #7f1d1d, #dc2626);
    box-shadow: 0 0 28px rgba(239, 68, 68, 0.85);
  }

  /* ESTADO LIGADO: VERDE */
  .estop-btn.state-on {
    background: linear-gradient(135deg, #065f46, #10b981);
    border: 2px solid #34d399;
    box-shadow: 0 0 20px rgba(16, 185, 129, 0.6);
    animation: estopPulseGreen 2s infinite alternate;
  }
  .estop-btn.state-on:hover {
    background: linear-gradient(135deg, #047857, #059669);
    box-shadow: 0 0 28px rgba(16, 185, 129, 0.85);
  }
  .estop-btn:active { transform: scale(0.97); }

  @keyframes estopPulseRed {
    from { box-shadow: 0 0 10px rgba(239, 68, 68, 0.4); }
    to { box-shadow: 0 0 25px rgba(239, 68, 68, 0.8); }
  }

  @keyframes estopPulseGreen {
    from { box-shadow: 0 0 10px rgba(16, 185, 129, 0.4); }
    to { box-shadow: 0 0 25px rgba(16, 185, 129, 0.8); }
  }

  .slider-box {
    display: flex;
    flex-direction: column;
    gap: 6px;
    background: rgba(15, 23, 42, 0.5);
    border: 1px solid var(--panel-border);
    padding: 8px 10px;
  }
  .slider-row {
    display: flex;
    justify-content: space-between;
    font-family: var(--font-mono);
    font-size: 0.72rem;
  }
  input[type=range] {
    width: 100%;
    accent-color: var(--cyan);
    cursor: pointer;
  }
  .speed-presets {
    display: flex;
    gap: 6px;
  }
  .preset-btn {
    flex: 1;
    background: rgba(30, 41, 59, 0.6);
    border: 1px solid var(--panel-border);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 0.65rem;
    padding: 4px;
    cursor: pointer;
  }
  .preset-btn:hover { background: rgba(0, 242, 254, 0.15); color: #fff; }

  .dpad-container {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    grid-template-rows: repeat(3, 74px);
    gap: 10px;
    max-width: 275px;
    width: 100%;
    margin: 8px auto;
  }
  .dpad-btn {
    background: rgba(15, 23, 42, 0.85);
    border: 1.5px solid var(--panel-border);
    color: var(--cyan);
    font-size: 2.1rem;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.1s ease;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5);
  }
  .dpad-btn:hover {
    background: rgba(0, 242, 254, 0.25);
    border-color: var(--cyan);
    box-shadow: 0 0 16px var(--cyan-glow);
  }
  .dpad-btn:active, .dpad-btn.pressed {
    background: var(--cyan);
    color: #040812;
    box-shadow: 0 0 25px var(--cyan-glow);
    transform: scale(0.93);
  }
  .dpad-center {
    background: rgba(239, 68, 68, 0.2);
    border-color: rgba(239, 68, 68, 0.6);
    color: var(--red);
    font-size: 1.8rem;
    font-weight: 900;
  }
  .dpad-center:hover {
    background: rgba(239, 68, 68, 0.4);
    border-color: #f87171;
    color: #fff;
    box-shadow: 0 0 16px var(--red-glow);
  }
  .dpad-center:active, .dpad-center.pressed {
    background: var(--red);
    color: #fff;
    box-shadow: 0 0 25px var(--red-glow);
  }

  .terminal-box {
    background: #020409;
    border: 1px solid var(--panel-border);
    height: 140px;
    padding: 6px 8px;
    font-family: var(--font-mono);
    font-size: 0.68rem;
    color: #38bdf8;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  .log-line { line-height: 1.3; }
  .log-time { color: var(--text-dim); }
  .log-tag-rx { color: var(--green); }
  .log-tag-tx { color: var(--cyan); }
  .log-tag-warn { color: var(--amber); }
  .log-tag-alert { color: var(--red); font-weight: 700; }
</style>
</head>
<body>

<div class="hud-container">
  <!-- TOP STATUS BAR -->
  <header class="tech-chamfer">
    <div class="brand-block">
      <div class="brand-titles">
        <h1>MEGATRON // TACTICAL HUD</h1>
        <p>AUTONOMOUS SLAM &amp; TELEMETRY SYSTEM</p>
      </div>
    </div>

    <div class="telemetry-status-strip">
      <div id="connBadge" class="status-badge tech-chamfer" title="Clique para reconectar/parear">
        <span class="pulse-dot"></span>
        <span id="connText">ONLINE (14ms)</span>
      </div>

      <div class="battery-gauge tech-chamfer">
        <div class="bat-icon"><div id="batFill" class="bat-fill"></div></div>
        <div id="batText" class="bat-text">88% (7.8V)</div>
      </div>

      <div class="battery-gauge tech-chamfer" style="border-color: rgba(16, 185, 129, 0.4);">
        <span style="font-family: var(--font-mono); font-size: 0.65rem; color: var(--text-muted);">MODO:</span>
        <span id="modeBadgeText" style="color: var(--green); font-weight: 800; font-size: 0.72rem;">AUTO SLAM</span>
      </div>
    </div>
  </header>

  <!-- DASHBOARD GRID -->
  <div class="dashboard-grid">
    <!-- LEFT COLUMN: TELEMETRY & SENSORS -->
    <div class="col-left" style="display: flex; flex-direction: column; gap: 10px;">
      <!-- POSE & SLAM CARD -->
      <div class="hud-card tech-chamfer">
        <div class="card-header">
          <span class="card-title">TELEMETRIA &amp; POSE</span>
          <span class="card-tag">POS_EST_V3</span>
        </div>
        <div class="telemetry-row">
          <span class="tel-label">COORDENADA (X, Y)</span>
          <span id="telCoords" class="tel-val neon">+0.00, +0.00 m</span>
        </div>
        <div class="telemetry-row">
          <span class="tel-label">ORIENTACAO CARD</span>
          <span id="telHeading" class="tel-val">000&deg; [NORTE]</span>
        </div>
        <div class="telemetry-row">
          <span class="tel-label">CELULAS MAPEADAS</span>
          <span id="telExplored" class="tel-val">0 cel (0.0 m&sup2;)</span>
        </div>
        <div class="telemetry-row">
          <span class="tel-label">ESTADO ATUAL</span>
          <span id="telStatus" class="tel-val" style="color: var(--green);">EXPLORANDO</span>
        </div>
      </div>

      <!-- ROBOTIC MODEL REPRESENTATION (ROBOTIC ID) -->
      <div class="hud-card tech-chamfer">
        <div class="card-header">
          <span class="card-title">REPRESENTACAO DO MODELO</span>
          <span class="card-tag">ROBOTIC_ID</span>
        </div>
        <div class="model-view-box tech-chamfer">
          <img src="/robot.jpg?v=2" alt="MEGATRON Robotic ID" class="model-img" />
          <div class="model-overlay-tag">MEGATRON // REV-3</div>
          <div class="model-scanline"></div>
        </div>
      </div>

      <!-- ARTIFICIAL HORIZON (IMU) -->
      <div class="hud-card tech-chamfer">
        <div class="card-header">
          <span class="card-title">HORIZONTE ARTIFICIAL (IMU)</span>
          <span class="card-tag">ATTITUDE_AHRS</span>
        </div>
        <div class="imu-wrapper tech-chamfer">
          <div id="imuHorizon" class="horizon-sky"></div>
          <div class="horizon-line"></div>
          <div class="horizon-reticle"></div>
        </div>
        <div style="display: flex; justify-content: space-around; font-family: var(--font-mono); font-size: 0.7rem; margin-top: 4px;">
          <span>PITCH: <b id="valPitch" style="color:var(--cyan)">0.0&deg;</b></span>
          <span>ROLL: <b id="valRoll" style="color:var(--cyan)">0.0&deg;</b></span>
          <span>YAW: <b id="valYaw" style="color:var(--cyan)">0.0&deg;</b></span>
        </div>
      </div>

      <!-- QUAD-QUADRANT PROXIMITY SONAR -->
      <div class="hud-card tech-chamfer">
        <div class="card-header">
          <span class="card-title">PROXIMIDADE // 4 QUADRANTES</span>
          <span class="card-tag">HC-SR04_FOV</span>
        </div>
        <div class="proximity-grid">
          <div class="prox-cell tech-chamfer">
            <span class="prox-label">FRENTE (FWD)</span>
            <span id="proxFront" class="prox-val">-- cm</span>
            <div class="prox-bar"><div id="barFront" class="prox-fill"></div></div>
          </div>
          <div class="prox-cell tech-chamfer">
            <span class="prox-label">RE (BACK)</span>
            <span id="proxBack" class="prox-val">-- cm</span>
            <div class="prox-bar"><div id="barBack" class="prox-fill"></div></div>
          </div>
          <div class="prox-cell tech-chamfer">
            <span class="prox-label">ESQUERDA (LFT)</span>
            <span id="proxLeft" class="prox-val">-- cm</span>
            <div class="prox-bar"><div id="barLeft" class="prox-fill"></div></div>
          </div>
          <div class="prox-cell tech-chamfer">
            <span class="prox-label">DIREITA (RGT)</span>
            <span id="proxRight" class="prox-val">-- cm</span>
            <div class="prox-bar"><div id="barRight" class="prox-fill"></div></div>
          </div>
        </div>
      </div>
    </div>

    <!-- CENTER COLUMN: RADAR / SLAM CANVAS HERO -->
    <div class="hud-card radar-card tech-chamfer-lg col-center">
      <canvas id="radarCanvas" width="700" height="700"></canvas>

      <div class="canvas-tools-overlay">
        <button id="btnZoomIn" class="hud-tool-btn tech-chamfer" title="Zoom +">+</button>
        <button id="btnZoomOut" class="hud-tool-btn tech-chamfer" title="Zoom -">&minus;</button>
        <button id="btnCenterRobot" class="hud-tool-btn tech-chamfer" title="Centralizar Robo">&#9678;</button>
        <button id="btnClearWp" class="hud-tool-btn tech-chamfer" title="Limpar Waypoint">&times;</button>
      </div>

      <div id="wpBadge" class="waypoint-prompt-badge tech-chamfer" style="display: none;">
        <span style="color: var(--amber);">&#9889; WAYPOINT ALVO:</span>
        <span id="wpCoordsText">X: 0.00, Y: 0.00</span>
      </div>
    </div>

    <!-- RIGHT COLUMN: MODES, CONTROLS & LOGS -->
    <div class="col-right" style="display: flex; flex-direction: column; gap: 10px;">
      <!-- MODES & ESTOP -->
      <div class="hud-card tech-chamfer card-modes">
        <div class="card-header">
          <span class="card-title">MODO OPERACIONAL</span>
          <span class="card-tag">SYS_CTRL</span>
        </div>
        <div class="mode-buttons-grid">
          <button id="btnModeAuto" class="mode-btn tech-chamfer">[AUTO] SLAM</button>
          <button id="btnModeManual" class="mode-btn tech-chamfer active">[MANUAL]</button>
          <button id="btnModeRTH" class="mode-btn tech-chamfer">[RTH] RETORNO</button>
          <button id="btnResetMap" class="mode-btn tech-chamfer">[RESET] MAPA</button>
          <button id="btnEstop" class="estop-btn state-on tech-chamfer">[LIGADO] PARADA DE EMERGENCIA</button>
        </div>
      </div>

      <!-- SPEED SLIDER -->
      <div class="hud-card tech-chamfer card-speed">
        <div class="card-header">
          <span class="card-title">POTENCIA / VELOCIDADE PWM</span>
          <span id="speedValText" class="card-tag" style="color: var(--cyan);">160 (63%)</span>
        </div>
        <div class="slider-box tech-chamfer">
          <input id="sliderSpeed" type="range" min="80" max="255" value="160">
          <div class="speed-presets">
            <button class="preset-btn tech-chamfer" onclick="setSpeedPreset(120)">35% ECO</button>
            <button class="preset-btn tech-chamfer" onclick="setSpeedPreset(170)">65% CRUISE</button>
            <button class="preset-btn tech-chamfer" onclick="setSpeedPreset(255)">100% TURBO</button>
          </div>
        </div>
      </div>

      <!-- TACTICAL D-PAD -->
      <div class="hud-card tech-chamfer card-dpad">
        <div class="card-header">
          <span class="card-title">MANUAL DRIVE (D-PAD / WASD)</span>
          <span class="card-tag">JOY_V2</span>
        </div>
        <div class="dpad-container">
          <div></div>
          <button id="btnFwd" class="dpad-btn tech-chamfer" data-dir="F">&#9650;</button>
          <div></div>
          <button id="btnLeft" class="dpad-btn tech-chamfer" data-dir="L">&#9664;</button>
          <button id="btnStop" class="dpad-btn dpad-center tech-chamfer" data-dir="STOP">&#9632;</button>
          <button id="btnRight" class="dpad-btn tech-chamfer" data-dir="R">&#9654;</button>
          <div></div>
          <button id="btnBack" class="dpad-btn tech-chamfer" data-dir="B">&#9660;</button>
          <div></div>
        </div>
      </div>

      <!-- TERMINAL LOGS -->
      <div class="hud-card tech-chamfer card-logs">
        <div class="card-header">
          <span class="card-title">LOGS DO SISTEMA</span>
          <button onclick="clearLogs()" style="background:none; border:none; color:var(--text-dim); font-family:var(--font-mono); font-size:0.6rem; cursor:pointer;">[LIMPAR]</button>
        </div>
        <div id="terminal" class="terminal-box tech-chamfer">
          <div class="log-line"><span class="log-time">[00:00:00]</span> <span class="log-tag-rx">[SYS]</span> MEGATRON HUD inicializado.</div>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
// Redireciona automaticamente para o IP real se o Windows/Android abrir no dominio de teste do captive portal
if (window.location.hostname !== '192.168.4.1' && window.location.hostname !== 'megatron.local' && window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1') {
  window.location.href = 'http://192.168.4.1/';
}

// Canvas & Context
const canvas = document.getElementById('radarCanvas');
const ctx = canvas.getContext('2d');

// UI Elements
const connBadge = document.getElementById('connBadge');
const connText = document.getElementById('connText');
const batFill = document.getElementById('batFill');
const batText = document.getElementById('batText');
const modeBadgeText = document.getElementById('modeBadgeText');
const telCoords = document.getElementById('telCoords');
const telHeading = document.getElementById('telHeading');
const telExplored = document.getElementById('telExplored');
const telStatus = document.getElementById('telStatus');

const imuHorizon = document.getElementById('imuHorizon');
const valPitch = document.getElementById('valPitch');
const valRoll = document.getElementById('valRoll');
const valYaw = document.getElementById('valYaw');

const proxFront = document.getElementById('proxFront');
const proxBack = document.getElementById('proxBack');
const proxLeft = document.getElementById('proxLeft');
const proxRight = document.getElementById('proxRight');
const barFront = document.getElementById('barFront');
const barBack = document.getElementById('barBack');
const barLeft = document.getElementById('barLeft');
const barRight = document.getElementById('barRight');

const sliderSpeed = document.getElementById('sliderSpeed');
const speedValText = document.getElementById('speedValText');
const terminal = document.getElementById('terminal');

const btnModeAuto = document.getElementById('btnModeAuto');
const btnModeManual = document.getElementById('btnModeManual');
const btnModeRTH = document.getElementById('btnModeRTH');
const btnResetMap = document.getElementById('btnResetMap');
const btnEstop = document.getElementById('btnEstop');

const btnZoomIn = document.getElementById('btnZoomIn');
const btnZoomOut = document.getElementById('btnZoomOut');
const btnCenterRobot = document.getElementById('btnCenterRobot');
const btnClearWp = document.getElementById('btnClearWp');
const wpBadge = document.getElementById('wpBadge');
const wpCoordsText = document.getElementById('wpCoordsText');

// WebSocket & System State
let ws = null;
let currentMode = "manual";
let lastActiveMode = "manual";
let currentSpeed = 160;
let lastPingSent = 0;
let activeTargetWaypoint = null;

// Power Button State Controller
function updatePowerButton(isLigado) {
  if (isLigado) {
    btnEstop.className = 'estop-btn state-on tech-chamfer';
    btnEstop.textContent = '[LIGADO] PARADA DE EMERGENCIA';
    btnEstop.title = 'Clique para PARADA DE EMERGENCIA / DESLIGAR';
  } else {
    btnEstop.className = 'estop-btn state-off tech-chamfer';
    btnEstop.textContent = '[DESLIGADO] CLIQUE P/ LIGAR';
    btnEstop.title = 'Clique para LIGAR / DESTRAVAR o robo';
  }
}

// Robot Pose & Telemetry State
let robotPose = { x: 0, y: 0, dir: 0 };
let imuData = { roll: 0, pitch: 0, yaw: 0 };
let mappedCells = [];
let pathPoints = [[0, 0]];

// Polar Tactical Radar (LiDAR / Sonar Sweep) State
let radarObstacles = [];
let sweptSectors = [];
let activeScannerAngle = 90;
let lastRealDataTime = 0;
let simSweepAngle = 30;
let simSweepDir = 1.4;
let simLastStepTime = 0;

// Canvas Navigation & Drag
let zoomLevel = 1.0;
let panOffset = { x: 0, y: 0 };
let isPanning = false;
let panStart = { x: 0, y: 0 };

// Log function
function logMsg(tag, msg, type = 'info') {
  const now = new Date();
  const timeStr = now.toTimeString().split(' ')[0] + '.' + String(now.getMilliseconds()).padStart(3, '0');
  const div = document.createElement('div');
  div.className = 'log-line';
  let tagClass = 'log-tag-rx';
  if (type === 'tx') tagClass = 'log-tag-tx';
  else if (type === 'warn') tagClass = 'log-tag-warn';
  else if (type === 'alert') tagClass = 'log-tag-alert';
  div.innerHTML = `<span class="log-time">[${timeStr}]</span> <span class="${tagClass}">[${tag}]</span> ${msg}`;
  terminal.appendChild(div);
  terminal.scrollTop = terminal.scrollHeight;
}
function clearLogs() { terminal.innerHTML = ''; }

// WEBSOCKET SETUP
function initWebSocket() {
  let wsHost = '192.168.4.1';
  if (window.location.hostname === '192.168.4.1' || window.location.hostname === 'megatron.local' || window.location.hostname === 'localhost') {
    wsHost = window.location.host;
  }
  const wsUrl = (window.location.protocol === 'https:' ? 'wss://' : 'ws://') + wsHost + '/ws';
  logMsg('SYS', 'Conectando WebSocket em ' + wsUrl + '...');

  try {
    if (ws) { ws.close(); }
  } catch(e) {}

  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    connBadge.className = 'status-badge tech-chamfer';
    connText.textContent = 'ONLINE';
    logMsg('WS', 'Conexao WebSocket estabelecida!', 'tx');
    sendPing();
  };

  ws.onclose = () => {
    connBadge.className = 'status-badge tech-chamfer disconnected';
    connText.textContent = 'DESCONECTADO';
    logMsg('WS', 'WebSocket desconectado. Reconectando em 2s...', 'warn');
    setTimeout(initWebSocket, 2000);
  };

  ws.onerror = err => {
    logMsg('WS', 'Erro de rede no WebSocket', 'alert');
  };

  ws.onmessage = evt => {
    try {
      const data = JSON.parse(evt.data);
      if (data.type === 'pong') {
        const pingMs = Math.round(performance.now() - lastPingSent);
        connText.textContent = `ONLINE (${pingMs}ms)`;
        setTimeout(sendPing, 3000);
      } else {
        processTelemetry(data);
      }
    } catch (e) {
      logMsg('ERR', 'Falha ao decodificar JSON: ' + e.message, 'alert');
    }
  };
}

connBadge.addEventListener('click', () => {
  logMsg('SYS', 'Reconexao manual solicitada...', 'tx');
  initWebSocket();
});

function sendWs(obj) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(obj));
    if (obj.cmd !== 'ping') {
      logMsg('TX', JSON.stringify(obj), 'tx');
    }
  }
}

function sendPing() {
  lastPingSent = performance.now();
  sendWs({ cmd: 'ping', t: Date.now() });
}

// TELEMETRY HANDLER
function processTelemetry(data) {
  if (typeof data.x === 'number') robotPose.x = data.x;
  if (typeof data.y === 'number') robotPose.y = data.y;
  if (typeof data.dir === 'number') robotPose.dir = data.dir;

  const cardDirs = ['NORTE \u25B2 (0\u00B0)', 'LESTE \u25B6 (90\u00B0)', 'SUL \u25BC (180\u00B0)', 'OESTE \u25C0 (270\u00B0)'];
  telCoords.textContent = `${robotPose.x >= 0 ? '+' : ''}${robotPose.x.toFixed(2)}, ${robotPose.y >= 0 ? '+' : ''}${robotPose.y.toFixed(2)} m`;
  telHeading.textContent = `${robotPose.dir}\u00B0 [${cardDirs[Math.floor(robotPose.dir / 90) % 4] || 'N'}]`;

  if (typeof data.enabled === 'boolean') {
    updatePowerButton(data.enabled);
  } else if (data.status) {
    const isEstop = data.status.includes('ESTOP') || data.status.includes('EMERG');
    updatePowerButton(!isEstop);
  }

  if (data.status) {
    telStatus.textContent = data.status.toUpperCase();
    modeBadgeText.textContent = data.status.toUpperCase();
    if (data.status.includes('AUTO')) {
      [btnModeAuto, btnModeManual, btnModeRTH].forEach(b => b.classList.remove('active'));
      btnModeAuto.classList.add('active');
      lastActiveMode = 'auto';
    } else if (data.status.includes('MANUAL')) {
      [btnModeAuto, btnModeManual, btnModeRTH].forEach(b => b.classList.remove('active'));
      btnModeManual.classList.add('active');
      lastActiveMode = 'manual';
    } else if (data.status.includes('RTH')) {
      [btnModeAuto, btnModeManual, btnModeRTH].forEach(b => b.classList.remove('active'));
      btnModeRTH.classList.add('active');
      lastActiveMode = 'rth';
    }
  }

  if (typeof data.explored === 'number') {
    telExplored.textContent = `${data.explored} cel (${(data.explored * 0.04).toFixed(1)} m\u00B2)`;
  }

  if (typeof data.battery === 'number') {
    const pct = Math.max(0, Math.min(100, Math.round(data.battery)));
    batFill.style.width = pct + '%';
    const volt = data.voltage ? data.voltage.toFixed(1) : (6.4 + (pct / 100) * 2.0).toFixed(1);
    batText.textContent = `${pct}% (${volt}V)`;
    if (pct < 25) batFill.style.background = 'var(--red)';
    else if (pct < 50) batFill.style.background = 'var(--amber)';
    else batFill.style.background = 'var(--green)';
  }

  if (typeof data.pitch === 'number') imuData.pitch = data.pitch;
  if (typeof data.roll === 'number') imuData.roll = data.roll;
  if (typeof data.yaw === 'number') imuData.yaw = data.yaw;
  valPitch.textContent = imuData.pitch.toFixed(1) + '\u00B0';
  valRoll.textContent = imuData.roll.toFixed(1) + '\u00B0';
  valYaw.textContent = imuData.yaw.toFixed(1) + '\u00B0';
  imuHorizon.style.transform = `rotate(${-imuData.roll}deg) translateY(${imuData.pitch * 1.5}px)`;

  updateProxQuad(proxFront, barFront, data.dist_fwd);
  updateProxQuad(proxBack, barBack, data.dist_back);
  updateProxQuad(proxLeft, barLeft, data.dist_left);
  updateProxQuad(proxRight, barRight, data.dist_right);

  // Hook telemetry directly to Polar Radar
  if (data.radar_angle !== undefined && data.radar_dist !== undefined) {
    window.updateRadarObstacle(data.radar_angle, data.radar_dist);
  }
  if (typeof data.dist_fwd === 'number' && data.dist_fwd > 0 && data.dist_fwd < 400) {
    window.updateRadarObstacle(0, data.dist_fwd);
  }
  if (typeof data.dist_left === 'number' && data.dist_left > 0 && data.dist_left < 400) {
    window.updateRadarObstacle(270, data.dist_left);
  }
  if (typeof data.dist_right === 'number' && data.dist_right > 0 && data.dist_right < 400) {
    window.updateRadarObstacle(90, data.dist_right);
  }
  if (typeof data.dist_back === 'number' && data.dist_back > 0 && data.dist_back < 400) {
    window.updateRadarObstacle(180, data.dist_back);
  }

  if (typeof data.speed === 'number' && !isDraggingSpeed) {
    currentSpeed = data.speed;
    sliderSpeed.value = data.speed;
    speedValText.textContent = `${data.speed} (${Math.round((data.speed/255)*100)}%)`;
  }

  if (data.path && Array.isArray(data.path)) pathPoints = data.path;
  if (data.cells && Array.isArray(data.cells)) mappedCells = data.cells;
}

function updateProxQuad(labelEl, barEl, distCm) {
  if (distCm === undefined || distCm === null) return;
  if (distCm < 0 || distCm > 400) {
    labelEl.textContent = '> 400 cm';
    barEl.style.width = '100%';
    barEl.style.background = 'var(--cyan)';
  } else {
    labelEl.textContent = distCm.toFixed(1) + ' cm';
    const pct = Math.min(100, Math.max(5, (distCm / 100) * 100));
    barEl.style.width = pct + '%';
    if (distCm <= 25) barEl.style.background = 'var(--red)';
    else if (distCm <= 45) barEl.style.background = 'var(--amber)';
    else barEl.style.background = 'var(--green)';
  }
}

// SPEED SLIDER (CONTROLE EM TEMPO REAL)
let speedThrottleTimer = null;
let isDraggingSpeed = false;

function sendSpeedWs(val) {
  clearTimeout(speedThrottleTimer);
  speedThrottleTimer = setTimeout(() => {
    sendWs({ cmd: 'speed', value: val });
  }, 40);
}

sliderSpeed.addEventListener('mousedown', () => { isDraggingSpeed = true; });
sliderSpeed.addEventListener('touchstart', () => { isDraggingSpeed = true; }, { passive: true });
window.addEventListener('mouseup', () => { isDraggingSpeed = false; });
window.addEventListener('touchend', () => { isDraggingSpeed = false; });

sliderSpeed.addEventListener('input', e => {
  currentSpeed = parseInt(e.target.value);
  speedValText.textContent = `${currentSpeed} (${Math.round((currentSpeed/255)*100)}%)`;
  sendSpeedWs(currentSpeed);
});

function setSpeedPreset(val) {
  sliderSpeed.value = val;
  currentSpeed = val;
  speedValText.textContent = `${currentSpeed} (${Math.round((currentSpeed/255)*100)}%)`;
  sendWs({ cmd: 'speed', value: val });
}

// OPERATIONAL MODES
btnModeAuto.addEventListener('click', () => { 
  lastActiveMode = 'auto';
  setMode('auto'); 
});

btnModeManual.addEventListener('click', () => { 
  lastActiveMode = 'manual';
  setMode('manual'); 
});

btnModeRTH.addEventListener('click', () => { 
  lastActiveMode = 'rth';
  setMode('rth'); 
});

btnEstop.addEventListener('click', () => {
  const isCurrentlyOn = btnEstop.classList.contains('state-on');
  if (isCurrentlyOn) {
    // Esta Ligado -> Aciona E-STOP (DESLIGADO)
    if (currentMode !== 'estop') {
      lastActiveMode = currentMode;
    }
    sendWs({ cmd: 'estop' });
    setMode('estop');
    updatePowerButton(false);
    logMsg('ALERT', 'PARADA DE EMERGENCIA! MOTORES DESLIGADOS.', 'alert');
    if (navigator.vibrate) navigator.vibrate([100, 50, 100]);
  } else {
    // Esta Desligado -> Destrava e liga no modo que estava selecionado (manual ou auto)
    const modeToRestore = lastActiveMode || 'manual';
    setMode(modeToRestore);
    updatePowerButton(true);
    logMsg('SYS', `ROBO LIGADO (Modo ${modeToRestore.toUpperCase()} ativado)...`, 'tx');
    if (navigator.vibrate) navigator.vibrate(60);
  }
});

function setMode(mode) {
  currentMode = mode;
  [btnModeAuto, btnModeManual, btnModeRTH].forEach(b => b.classList.remove('active'));
  if (mode === 'auto') {
    btnModeAuto.classList.add('active');
    updatePowerButton(true);
  } else if (mode === 'manual') {
    btnModeManual.classList.add('active');
    updatePowerButton(true); // NO MODO MANUAL O ROBO CONTINUA LIGADO!
  } else if (mode === 'rth') {
    btnModeRTH.classList.add('active');
    updatePowerButton(true);
  } else if (mode === 'estop') {
    updatePowerButton(false); // APENAS NO E-STOP O ROBO FICA DESLIGADO (VERMELHO)!
  }
  sendWs({ cmd: 'mode', value: mode });
}

// RESET MAP
btnResetMap.addEventListener('click', () => {
  if (confirm('Confirma limpeza de mapa e trajetoria?')) {
    sendWs({ cmd: 'reset' });
    mappedCells = [];
    pathPoints = [[0, 0]];
    activeTargetWaypoint = null;
    wpBadge.style.display = 'none';
    logMsg('SYS', 'Mapa e percurso reiniciados.');
  }
});

// D-PAD & KEYBOARD DRIVE
const activeKeys = new Set();
const dirButtons = document.querySelectorAll('.dpad-btn');

dirButtons.forEach(btn => {
  const dir = btn.dataset.dir;
  const startAction = e => {
    e.preventDefault();
    btn.classList.add('pressed');
    sendWs({ cmd: 'move', dir: dir, speed: currentSpeed });
  };
  const endAction = e => {
    e.preventDefault();
    btn.classList.remove('pressed');
    if (dir !== 'STOP') sendWs({ cmd: 'move', dir: 'STOP', speed: 0 });
  };
  btn.addEventListener('mousedown', startAction);
  btn.addEventListener('mouseup', endAction);
  btn.addEventListener('touchstart', startAction, { passive: false });
  btn.addEventListener('touchend', endAction, { passive: false });
  btn.addEventListener('touchcancel', endAction, { passive: false });
});

window.addEventListener('keydown', e => {
  if (e.target.tagName === 'INPUT' || activeKeys.has(e.code)) return;
  activeKeys.add(e.code);
  let dir = null;
  if (e.code === 'KeyW' || e.code === 'ArrowUp') dir = 'F';
  else if (e.code === 'KeyS' || e.code === 'ArrowDown') dir = 'B';
  else if (e.code === 'KeyA' || e.code === 'ArrowLeft') dir = 'L';
  else if (e.code === 'KeyD' || e.code === 'ArrowRight') dir = 'R';
  else if (e.code === 'Space') dir = 'STOP';

  if (dir) {
    e.preventDefault();
    sendWs({ cmd: 'move', dir: dir, speed: currentSpeed });
    highlightDpad(dir, true);
  }
});

window.addEventListener('keyup', e => {
  activeKeys.delete(e.code);
  let dir = null;
  if (e.code === 'KeyW' || e.code === 'ArrowUp') dir = 'F';
  else if (e.code === 'KeyS' || e.code === 'ArrowDown') dir = 'B';
  else if (e.code === 'KeyA' || e.code === 'ArrowLeft') dir = 'L';
  else if (e.code === 'KeyD' || e.code === 'ArrowRight') dir = 'R';

  if (dir) {
    e.preventDefault();
    sendWs({ cmd: 'move', dir: 'STOP', speed: 0 });
    highlightDpad(dir, false);
  }
});

function highlightDpad(dir, pressed) {
  const el = document.querySelector(`.dpad-btn[data-dir="${dir}"]`);
  if (el) el.classList.toggle('pressed', pressed);
}

// ==============================================================================
// TACTICAL POLAR RADAR (LiDAR / Sonar Sweep) INTERACTION & RENDERING
// ==============================================================================
function handleRadarTap(clientX, clientY) {
  const rect = canvas.getBoundingClientRect();
  const screenX = (clientX - rect.left) * (canvas.width / rect.width);
  const screenY = (clientY - rect.top) * (canvas.height / rect.height);

  const baseR = Math.min(canvas.width, canvas.height) * 0.42;
  const R = baseR * zoomLevel;
  const scale = R / 5.0;
  const cx = canvas.width / 2 + panOffset.x;
  const cy = canvas.height / 2 + panOffset.y;

  const worldX = Math.round(((screenX - cx) / scale + robotPose.x) * 10) / 10;
  const worldY = Math.round(((cy - screenY) / scale + robotPose.y) * 10) / 10;

  activeTargetWaypoint = { x: worldX, y: worldY };
  wpBadge.style.display = 'flex';
  wpCoordsText.textContent = `X: ${worldX.toFixed(2)}, Y: ${worldY.toFixed(2)}`;
  sendWs({ cmd: 'goto', x: worldX, y: worldY });
  logMsg('WP', `Novo waypoint definido: [X: ${worldX}, Y: ${worldY}]`, 'tx');
}

canvas.addEventListener('click', e => {
  if (isPanning) return;
  handleRadarTap(e.clientX, e.clientY);
});

btnClearWp.addEventListener('click', () => {
  activeTargetWaypoint = null;
  wpBadge.style.display = 'none';
  logMsg('WP', 'Waypoint cancelado.', 'warn');
});

btnZoomIn.addEventListener('click', () => { zoomLevel = Math.min(2.5, zoomLevel * 1.25); });
btnZoomOut.addEventListener('click', () => { zoomLevel = Math.max(0.4, zoomLevel / 1.25); });
btnCenterRobot.addEventListener('click', () => { zoomLevel = 1.0; panOffset = { x: 0, y: 0 }; });

// PAN DRAG (MOUSE)
canvas.addEventListener('mousedown', e => {
  isPanning = true;
  panStart = { x: e.clientX - panOffset.x, y: e.clientY - panOffset.y };
});
window.addEventListener('mousemove', e => {
  if (!isPanning) return;
  panOffset.x = e.clientX - panStart.x;
  panOffset.y = e.clientY - panStart.y;
});
window.addEventListener('mouseup', () => { isPanning = false; });

// PAN DRAG & TAP (MOBILE TOUCH)
let touchTotalPan = 0;
let isTouchPanning = false;
canvas.addEventListener('touchstart', e => {
  if (e.touches.length === 1) {
    isTouchPanning = true;
    touchTotalPan = 0;
    panStart = { x: e.touches[0].clientX - panOffset.x, y: e.touches[0].clientY - panOffset.y };
  }
}, { passive: true });

window.addEventListener('touchmove', e => {
  if (!isTouchPanning || e.touches.length !== 1) return;
  const curX = e.touches[0].clientX - panStart.x;
  const curY = e.touches[0].clientY - panStart.y;
  touchTotalPan += Math.hypot(curX - panOffset.x, curY - panOffset.y);
  panOffset.x = curX;
  panOffset.y = curY;
}, { passive: true });

window.addEventListener('touchend', e => {
  if (isTouchPanning) {
    if (touchTotalPan < 8 && e.changedTouches && e.changedTouches.length > 0) {
      handleRadarTap(e.changedTouches[0].clientX, e.changedTouches[0].clientY);
    }
    isTouchPanning = false;
  }
});

// POLAR RADAR DATA PIPELINE
function addObstacleInternal(angleDeg, distanceCm, isReal) {
  if (distanceCm <= 3 || distanceCm > 500) return;
  const distM = distanceCm / 100.0;
  const now = performance.now();

  // Create granular organic point-cloud scatter (5-7 particles with radial & angular jitter)
  const scatter = [];
  const pCount = 5 + Math.floor(Math.random() * 3);
  for (let i = 0; i < pCount; i++) {
    scatter.push({
      dr: (Math.random() - 0.5) * 0.10 * distM,
      da: (Math.random() - 0.5) * (6 * Math.PI / 180)
    });
  }

  radarObstacles.push({
    angleDeg: angleDeg,
    distCm: distanceCm,
    distM: distM,
    timestamp: now,
    scatter: scatter
  });

  if (radarObstacles.length > 220) {
    radarObstacles.shift();
  }

  // Record swept sector corridor
  sweptSectors.push({
    angleDeg: angleDeg,
    distM: distM,
    timestamp: now
  });
  if (sweptSectors.length > 90) {
    sweptSectors.shift();
  }
}

// Global JS API requested by user
window.updateRadarObstacle = function(angleDeg, distanceCm) {
  lastRealDataTime = Date.now();
  activeScannerAngle = angleDeg;
  addObstacleInternal(angleDeg, distanceCm, true);
};

// MAIN TACTICAL RADAR RENDER LOOP (60 FPS)
function renderRadar() {
  requestAnimationFrame(renderRadar);

  const w = canvas.width;
  const h = canvas.height;
  const nowMs = performance.now();

  // 1. Deep Tactical Dark Background
  ctx.fillStyle = '#040812';
  ctx.fillRect(0, 0, w, h);

  const cx = w / 2 + panOffset.x;
  const cy = h / 2 + panOffset.y;
  const baseR = Math.min(w, h) * 0.42;
  const R = baseR * zoomLevel;
  const scale = R / 5.0; // 5 meters scale

  const bgGrad = ctx.createRadialGradient(cx, cy, 10, cx, cy, R * 1.05);
  bgGrad.addColorStop(0, '#061426');
  bgGrad.addColorStop(0.7, '#040d1a');
  bgGrad.addColorStop(1, '#040812');
  ctx.fillStyle = bgGrad;
  ctx.beginPath();
  ctx.arc(cx, cy, R * 1.05, 0, Math.PI * 2);
  ctx.fill();

  // 2. Stippled Cyber Dot Grid Matrix
  ctx.save();
  ctx.fillStyle = 'rgba(0, 242, 254, 0.07)';
  const dotSpacing = 28 * zoomLevel;
  if (dotSpacing >= 12) {
    const startX = cx - Math.floor(R / dotSpacing) * dotSpacing;
    const endX = cx + Math.floor(R / dotSpacing) * dotSpacing;
    const startY = cy - Math.floor(R / dotSpacing) * dotSpacing;
    const endY = cy + Math.floor(R / dotSpacing) * dotSpacing;
    for (let gx = startX; gx <= endX; gx += dotSpacing) {
      for (let gy = startY; gy <= endY; gy += dotSpacing) {
        const dx = gx - cx, dy = gy - cy;
        if (dx * dx + dy * dy <= R * R) {
          ctx.fillRect(gx - 0.75, gy - 0.75, 1.5, 1.5);
        }
      }
    }
  }
  ctx.restore();

  // 3. Translucent Free Area Sectors (Cyan / Emerald Gradient Wedges)
  ctx.save();
  sweptSectors = sweptSectors.filter(s => (nowMs - s.timestamp) < 5000);
  sweptSectors.forEach(sec => {
    const age = nowMs - sec.timestamp;
    const secAlpha = Math.max(0, 0.20 * (1.0 - age / 5000));
    const sRad = (sec.angleDeg - 90) * (Math.PI / 180);
    const wedgeHalf = (12 * Math.PI) / 180;
    const maxR = Math.min(R, (sec.distM / 5.0) * R);

    ctx.fillStyle = `rgba(16, 185, 129, ${secAlpha * 0.75})`;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.arc(cx, cy, maxR, sRad - wedgeHalf, sRad + wedgeHalf);
    ctx.closePath();
    ctx.fill();
  });
  ctx.restore();

  // 4. Concentric Metric Range Rings (1m, 2m, 3m, 4m, 5m)
  ctx.save();
  ctx.font = '10px "JetBrains Mono", monospace';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'bottom';

  for (let i = 1; i <= 5; i++) {
    const ri = (R / 5.0) * i;
    ctx.beginPath();
    ctx.arc(cx, cy, ri, 0, Math.PI * 2);
    if (i < 5) {
      ctx.strokeStyle = 'rgba(0, 242, 254, 0.16)';
      ctx.lineWidth = 1;
      ctx.setLineDash([3, 4]);
    } else {
      ctx.strokeStyle = 'rgba(0, 242, 254, 0.38)';
      ctx.lineWidth = 1.5;
      ctx.setLineDash([]);
    }
    ctx.stroke();

    ctx.fillStyle = 'rgba(0, 242, 254, 0.50)';
    ctx.fillText(i + 'm', cx + 4, cy - ri + 12);
    ctx.fillText(i + 'm', cx + 4, cy + ri - 3);
  }
  ctx.restore();

  // 5. Radial Angle Rays & Degree Outer Ticks
  ctx.save();
  const degSteps = [0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330];
  degSteps.forEach(deg => {
    const rad = (deg - 90) * (Math.PI / 180);
    const cos = Math.cos(rad);
    const sin = Math.sin(rad);

    ctx.beginPath();
    ctx.moveTo(cx + 26 * cos, cy + 26 * sin);
    ctx.lineTo(cx + R * cos, cy + R * sin);
    const isCardinal = (deg % 90 === 0);
    ctx.strokeStyle = isCardinal ? 'rgba(0, 242, 254, 0.22)' : 'rgba(0, 242, 254, 0.09)';
    ctx.lineWidth = isCardinal ? 1.2 : 0.8;
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(cx + (R - 4) * cos, cy + (R - 4) * sin);
    ctx.lineTo(cx + (R + 6) * cos, cy + (R + 6) * sin);
    ctx.strokeStyle = isCardinal ? 'rgba(0, 242, 254, 0.65)' : 'rgba(0, 242, 254, 0.32)';
    ctx.lineWidth = isCardinal ? 1.5 : 1;
    ctx.stroke();

    ctx.font = isCardinal ? 'bold 10px "Orbitron", monospace' : '9px "JetBrains Mono", monospace';
    ctx.fillStyle = isCardinal ? 'rgba(0, 242, 254, 0.85)' : 'rgba(0, 242, 254, 0.45)';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(deg + '\u00B0', cx + (R + 17) * cos, cy + (R + 17) * sin);
  });
  ctx.restore();

  // 6. Automatic Fallback Simulation (Runs if no real WebSocket data)
  if (Date.now() - lastRealDataTime > 2500) {
    simSweepAngle += simSweepDir;
    if (simSweepAngle >= 155) { simSweepAngle = 155; simSweepDir = -1.4; }
    else if (simSweepAngle <= 25) { simSweepAngle = 25; simSweepDir = 1.4; }
    activeScannerAngle = simSweepAngle;

    if (nowMs - simLastStepTime > 110) {
      simLastStepTime = nowMs;
      let d = 260;
      if (simSweepAngle >= 45 && simSweepAngle <= 75) d = 240 + Math.sin(simSweepAngle * 0.15) * 25;
      else if (simSweepAngle >= 125 && simSweepAngle <= 145) d = 22 + Math.sin(simSweepAngle * 0.25) * 3;
      else if (simSweepAngle >= 85 && simSweepAngle <= 105) d = 390 + Math.sin(simSweepAngle * 0.08) * 25;
      addObstacleInternal(simSweepAngle, d, false);
    }
  }

  // 7. Active Sweep Laser Beam & Trailing Phosphor Glow
  const sweepRad = (activeScannerAngle - 90) * (Math.PI / 180);
  ctx.save();
  const trailSpan = (simSweepDir >= 0 ? -1 : 1) * (26 * Math.PI / 180);
  const wedgeGrad = ctx.createRadialGradient(cx, cy, 2, cx, cy, R);
  wedgeGrad.addColorStop(0, 'rgba(0, 242, 254, 0.35)');
  wedgeGrad.addColorStop(0.7, 'rgba(0, 242, 254, 0.12)');
  wedgeGrad.addColorStop(1, 'rgba(0, 242, 254, 0.0)');

  ctx.fillStyle = wedgeGrad;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.arc(cx, cy, R, sweepRad + trailSpan, sweepRad, simSweepDir < 0);
  ctx.closePath();
  ctx.fill();

  ctx.strokeStyle = '#00f2fe';
  ctx.lineWidth = 2.0;
  ctx.shadowColor = '#00f2fe';
  ctx.shadowBlur = 14;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + R * Math.cos(sweepRad), cy + R * Math.sin(sweepRad));
  ctx.stroke();
  ctx.restore();

  // 8. Obstacle Point Cloud & Impact Arcs (Decay & Color-Coded Risk)
  ctx.save();
  radarObstacles = radarObstacles.filter(o => (nowMs - o.timestamp) < 4500);

  radarObstacles.forEach(obs => {
    const age = nowMs - obs.timestamp;
    const alpha = Math.max(0, 1.0 - (age / 4500));
    ctx.globalAlpha = alpha;

    const r = (obs.distM / 5.0) * R;
    const angRad = (obs.angleDeg - 90) * (Math.PI / 180);

    let strokeCol, partCol, shadowCol, glowSize, arcWidth;
    if (obs.distCm <= 25) {
      strokeCol = '#ef4444';
      partCol = '#f87171';
      shadowCol = '#ef4444';
      glowSize = 16;
      arcWidth = 3.5;
    } else if (obs.distCm <= 40) {
      strokeCol = '#f97316';
      partCol = '#fb923c';
      shadowCol = '#f97316';
      glowSize = 10;
      arcWidth = 2.8;
    } else {
      strokeCol = '#f59e0b';
      partCol = '#fbbf24';
      shadowCol = '#f59e0b';
      glowSize = 8;
      arcWidth = 2.4;
    }

    ctx.strokeStyle = strokeCol;
    ctx.lineWidth = arcWidth;
    ctx.shadowColor = shadowCol;
    ctx.shadowBlur = glowSize;

    // Curved Impact Arc
    const arcSpan = (7.5 * Math.PI) / 180;
    ctx.beginPath();
    ctx.arc(cx, cy, r, angRad - arcSpan, angRad + arcSpan);
    ctx.stroke();

    // Terminal cap beads
    const x1 = cx + r * Math.cos(angRad - arcSpan);
    const y1 = cy + r * Math.sin(angRad - arcSpan);
    const x2 = cx + r * Math.cos(angRad + arcSpan);
    const y2 = cy + r * Math.sin(angRad + arcSpan);
    ctx.fillStyle = strokeCol;
    ctx.beginPath(); ctx.arc(x1, y1, 2.2, 0, Math.PI * 2); ctx.fill();
    ctx.beginPath(); ctx.arc(x2, y2, 2.2, 0, Math.PI * 2); ctx.fill();

    // Granular Point Cloud
    ctx.fillStyle = partCol;
    ctx.shadowBlur = glowSize * 0.7;
    obs.scatter.forEach(sc => {
      const pr = Math.max(0, r + sc.dr * scale);
      const pa = angRad + sc.da;
      const px = cx + pr * Math.cos(pa);
      const py = cy + pr * Math.sin(pa);
      ctx.beginPath();
      ctx.arc(px, py, obs.distCm <= 25 ? 2.6 : 1.8, 0, Math.PI * 2);
      ctx.fill();
    });
  });
  ctx.restore();

  // 8.5. SLAM Occupancy Grid Map (Células Mapeadas & Obstáculos Persistentes)
  if (mappedCells && mappedCells.length > 0) {
    ctx.save();
    const cellSizeM = 0.15; // 15cm por célula
    const cellPx = Math.max(10, cellSizeM * scale);

    mappedCells.forEach(c => {
      const gx = c[0];
      const gy = c[1];
      const state = c[2];

      const wx = gx * cellSizeM;
      const wy = -gy * cellSizeM; // No GridMap dy = -1 é Norte (+Y)

      const px = cx + (wx - robotPose.x) * scale;
      const py = cy - (wy - robotPose.y) * scale;

      // Renderiza células no campo de visão
      const distFromCenter = Math.hypot(px - cx, py - cy);
      if (distFromCenter > R + 70) return;

      if (state === 2) {
        // OBSTÁCULO CONFIRMADO: Bloco tático neon vermelho vivo com glow
        ctx.fillStyle = 'rgba(239, 68, 68, 0.78)';
        ctx.strokeStyle = '#ef4444';
        ctx.lineWidth = 1.8;
        ctx.shadowColor = '#ef4444';
        ctx.shadowBlur = 10;
        ctx.fillRect(px - cellPx * 0.5, py - cellPx * 0.5, cellPx, cellPx);
        ctx.strokeRect(px - cellPx * 0.5, py - cellPx * 0.5, cellPx, cellPx);

        // Marca tática interna
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.7)';
        ctx.lineWidth = 1.2;
        ctx.beginPath();
        ctx.moveTo(px - cellPx * 0.3, py); ctx.lineTo(px + cellPx * 0.3, py);
        ctx.moveTo(px, py - cellPx * 0.3); ctx.lineTo(px, py + cellPx * 0.3);
        ctx.stroke();
      } else if (state === 1) {
        // ÁREA LIVRE: Piso suave esmeralda explorado
        ctx.fillStyle = 'rgba(16, 185, 129, 0.14)';
        ctx.strokeStyle = 'rgba(16, 185, 129, 0.28)';
        ctx.lineWidth = 1;
        ctx.shadowBlur = 0;
        ctx.fillRect(px - cellPx * 0.45, py - cellPx * 0.45, cellPx * 0.9, cellPx * 0.9);
        ctx.strokeRect(px - cellPx * 0.45, py - cellPx * 0.45, cellPx * 0.9, cellPx * 0.9);
      }
    });
    ctx.restore();
  }

  // 9. Robot Trajectory Trail (Dashed Amber Line & Nodes)
  if (pathPoints && pathPoints.length > 1) {
    ctx.save();
    ctx.strokeStyle = '#f59e0b';
    ctx.lineWidth = 2;
    ctx.setLineDash([4, 6]);
    ctx.shadowColor = '#f59e0b';
    ctx.shadowBlur = 6;
    ctx.beginPath();
    pathPoints.forEach((pt, i) => {
      const px = cx + (pt[0] - robotPose.x) * scale;
      const py = cy - (pt[1] - robotPose.y) * scale;
      if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
    });
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.fillStyle = '#fbbf24';
    pathPoints.forEach(pt => {
      const px = cx + (pt[0] - robotPose.x) * scale;
      const py = cy - (pt[1] - robotPose.y) * scale;
      ctx.beginPath();
      ctx.arc(px, py, 2.4, 0, Math.PI * 2);
      ctx.fill();
    });
    ctx.restore();
  }

  // 10. Active Target Waypoint
  if (activeTargetWaypoint) {
    const wx = cx + (activeTargetWaypoint.x - robotPose.x) * scale;
    const wy = cy - (activeTargetWaypoint.y - robotPose.y) * scale;
    const t = performance.now() * 0.003;

    ctx.save();
    ctx.strokeStyle = '#fbbf24';
    ctx.lineWidth = 2;
    ctx.shadowColor = '#fbbf24';
    ctx.shadowBlur = 12;

    ctx.beginPath();
    ctx.arc(wx, wy, 16 + Math.sin(t * 3) * 3, 0, Math.PI * 2);
    ctx.stroke();

    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.arc(wx, wy, 24, t, t + Math.PI * 2);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.beginPath();
    ctx.moveTo(wx - 28, wy); ctx.lineTo(wx + 28, wy);
    ctx.moveTo(wx, wy - 28); ctx.lineTo(wx, wy + 28);
    ctx.stroke();

    ctx.font = '10px "JetBrains Mono", monospace';
    ctx.fillStyle = '#fbbf24';
    ctx.fillText(`WP [${activeTargetWaypoint.x.toFixed(1)}, ${activeTargetWaypoint.y.toFixed(1)}]`, wx + 16, wy - 16);
    ctx.restore();
  }

  // 11. Central MEGATRON Icon & Heading Vector
  ctx.save();
  const headingRad = (robotPose.dir - 90) * (Math.PI / 180);

  // Outer Target Ring
  ctx.strokeStyle = '#00f2fe';
  ctx.lineWidth = 2;
  ctx.shadowColor = '#00f2fe';
  ctx.shadowBlur = 10;
  ctx.fillStyle = 'rgba(0, 242, 254, 0.12)';
  ctx.beginPath();
  ctx.arc(cx, cy, 22, 0, Math.PI * 2);
  ctx.fill();
  ctx.stroke();

  // Heading Proa Arrow Vector
  ctx.strokeStyle = '#00f2fe';
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  const arrowLen = 38;
  const ax = cx + arrowLen * Math.cos(headingRad);
  const ay = cy + arrowLen * Math.sin(headingRad);
  ctx.lineTo(ax, ay);
  ctx.stroke();

  // Arrowhead
  const headLen = 9;
  const headAngle = 26 * (Math.PI / 180);
  ctx.fillStyle = '#00f2fe';
  ctx.beginPath();
  ctx.moveTo(ax, ay);
  ctx.lineTo(ax - headLen * Math.cos(headingRad - headAngle), ay - headLen * Math.sin(headingRad - headAngle));
  ctx.lineTo(ax - headLen * 0.5 * Math.cos(headingRad), ay - headLen * 0.5 * Math.sin(headingRad));
  ctx.lineTo(ax - headLen * Math.cos(headingRad + headAngle), ay - headLen * Math.sin(headingRad + headAngle));
  ctx.closePath();
  ctx.fill();

  // Inner Body Chevron
  ctx.save();
  ctx.translate(cx, cy);
  ctx.rotate(headingRad + Math.PI / 2);
  ctx.fillStyle = '#00f2fe';
  ctx.beginPath();
  ctx.moveTo(0, -9);
  ctx.lineTo(6, 6);
  ctx.lineTo(0, 3);
  ctx.lineTo(-6, 6);
  ctx.closePath();
  ctx.fill();
  ctx.restore();

  // Center node dot
  ctx.fillStyle = '#fff';
  ctx.beginPath();
  ctx.arc(cx, cy, 2.5, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();

  // 12. Tactical HUD Corner Badges (Matching Reference Image)
  ctx.save();
  // Top-Right Cyber Bracket Box
  const bW = 126, bH = 48, bX = w - bW - 14, bY = 14;
  const bLen = 8;
  ctx.strokeStyle = '#f59e0b';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(bX, bY + bLen); ctx.lineTo(bX, bY); ctx.lineTo(bX + bLen, bY);
  ctx.moveTo(bX + bW - bLen, bY); ctx.lineTo(bX + bW, bY); ctx.lineTo(bX + bW, bY + bLen);
  ctx.moveTo(bX, bY + bH - bLen); ctx.lineTo(bX, bY + bH); ctx.lineTo(bX + bLen, bY + bH);
  ctx.moveTo(bX + bW - bLen, bY + bH); ctx.lineTo(bX + bW, bY + bH); ctx.lineTo(bX + bW, bY + bH - bLen);
  ctx.stroke();

  ctx.font = 'bold 9px "Orbitron", monospace';
  ctx.fillStyle = '#f59e0b';
  ctx.fillText('SR04 ACTIVE', bX + 12, bY + 16);
  ctx.font = '10px "JetBrains Mono", monospace';
  ctx.fillStyle = '#fff';
  ctx.fillText(`SERVO: ${Math.round(activeScannerAngle)}\u00B0`, bX + 12, bY + 30);
  ctx.font = '8px "JetBrains Mono", monospace';
  ctx.fillStyle = '#10b981';
  ctx.fillText('SCANNING...', bX + 12, bY + 41);

  // Top-Left Coordinates & Range
  ctx.font = '10px "JetBrains Mono", monospace';
  ctx.fillStyle = 'rgba(0, 242, 254, 0.7)';
  ctx.fillText(`X: ${robotPose.x >= 0 ? '+' : ''}${robotPose.x.toFixed(2)}m`, 16, 26);
  ctx.fillText(`Y: ${robotPose.y >= 0 ? '+' : ''}${robotPose.y.toFixed(2)}m`, 16, 40);

  // Bottom-Left Scale & SLAM Legend
  ctx.font = '9px "JetBrains Mono", monospace';
  ctx.fillStyle = 'rgba(0, 242, 254, 0.4)';
  ctx.fillText('ESCALA: 1 DIV = 1.0m (ALCANCE: 5m)', 16, h - 34);

  // Legenda do Mapa SLAM
  ctx.fillStyle = '#ef4444';
  ctx.fillRect(16, h - 22, 9, 9);
  ctx.fillStyle = '#f8fafc';
  ctx.fillText('OBSTÁCULO', 30, h - 14);

  ctx.fillStyle = '#10b981';
  ctx.fillRect(115, h - 22, 9, 9);
  ctx.fillStyle = '#f8fafc';
  ctx.fillText('LIVRE', 129, h - 14);

  ctx.fillStyle = '#f59e0b';
  ctx.fillRect(178, h - 22, 9, 9);
  ctx.fillStyle = '#f8fafc';
  ctx.fillText('TRAJETÓRIA', 192, h - 14);
  ctx.restore();
}

// START EVERYTHING
initWebSocket();
renderRadar();
</script>
</body>
</html>)rawliteral";

const size_t PAGE_HTML_LEN = sizeof(PAGE_HTML) - 1;
