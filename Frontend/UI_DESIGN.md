# IoT Electric Meter - UI Design Document

## Overview

Modern dark-themed dashboard for monitoring electricity consumption. Designed with a clean, minimal aesthetic featuring teal accents on deep dark backgrounds.

---

## Color Palette

### Backgrounds

```markdown
| Color Name      | Hex Code  | Usage                              |
|-----------------|-----------|-------------------------------------|
| Deep Dark       | `#080d14` | Main background                    |
| Card Dark       | `#0f1623` | Cards, gauge background           |
| Border Dark     | `#1a2233` | Card borders, dividers            |
```

### Brand Accent

```markdown
| Color Name      | Hex Code  | Usage                              |
|-----------------|-----------|-------------------------------------|
| Primary Teal    | `#00d2c8` | Primary accent, buttons, gauge     |
| Teal Light      | `#00f2e8` | Gradient highlight                |
| Teal Dark       | `#00a39b` | Button gradient end               |
```

### Text

```markdown
| Color Name      | Hex Code  | Usage                              |
|-----------------|-----------|-------------------------------------|
| White           | `#f8fafc` | Primary text, values               |
| Light Gray      | `#94a3b8` | Secondary text, labels, currency   |
| Muted Gray      | `#64748b` | Tertiary text                      |
```

### Semantic Colors

```markdown
| Color Name      | Hex Code  | Usage                              |
|-----------------|-----------|-------------------------------------|
| Teal Tint       | `rgba(0,210,200,0.1)` | Badges, backgrounds |
| Teal Border     | `rgba(0,210,200,0.2)` | Badge borders       |
```

---

## Typography

```markdown
| Element           | Size    | Weight   | Color      | Letter Spacing |
|-------------------|---------|----------|------------|----------------|
| Header Title      | 18px    | 900      | #00d2c8    | 1.5px          |
| Gauge Value       | 64px    | Bold     | #f8fafc    | -              |
| Gauge Unit        | 20px    | 600      | #00d2c8    | -              |
| Gauge Label       | 12px    | 600      | #94a3b8    | 1px            |
| Cost Value        | 32px    | 700      | #f8fafc    | -              |
| Card Value        | 28px    | 700      | #f8fafc    | -              |
| Grid Value        | 20px    | 700      | #f8fafc    | -              |
| Button Text       | 16px    | 600      | #f8fafc    | -              |
| Card Label        | 12px    | 700      | #94a3b8    | 1px            |
| Nav Text          | 10px    | 500-700  | #00d2c8/#94a3b8 | -        |
| Trend Badge      | 10px    | Bold     | #00d2c8    | -              |
```

---

## Components

### 1. Header

```
┌─────────────────────────────────────────┐
│ ⚡ ENERGY PULSE                    [👤] │
└─────────────────────────────────────────┘
```

- **Elements**: Bolt icon (Lucide), "ENERGY PULSE" title, Avatar placeholder
- **Padding**: 24px horizontal, 16px vertical
- **Background**: Transparent (inherits #080d14)

---

### 2. Circular Gauge

```
        ╭───────────────╮
      ╭─│               │─╮
     │  │  CURRENT MONTH │  │
     │  │                │  │
     │  │      482       │  │
     │  │      kWh       │  │
     │  │                │  │
     │  │ [↗ 12% ABOVE] │  │
      ╰─│               │─╯
        ╰───────────────╯
```

- **Circle Size**: 70% of screen width
- **Stroke Width**: 15px
- **Gradient**: #00d2c8 → #00f2e8
- **Background Ring**: #0f1623
- **Shadow**: Teal glow (0.2 opacity, 20px radius)
- **Progress**: 65% (configurable)

---

### 3. Cost Display

```
┌─────────────────────────────────────────┐
│         Estimated Monthly Cost          │
│              1,245.50 EGP               │
└─────────────────────────────────────────┘
```

- **Alignment**: Center
- **Spacing**: 40px margin bottom

---

### 4. Balance Card

```
┌─────────────────────────────────────────┐
│  PREPAID BALANCE         [+ TOP UP]    │
│  458.00 EGP                           │
└─────────────────────────────────────────┘
```

- **Background**: #0f1623
- **Border**: 1px #1a2233
- **Border Radius**: 24px
- **Button**: Gradient #00d2c8 → #00a39b
- **Button Border Radius**: 16px
- **Shadow**: Teal (0.3 opacity, 8px radius)

---

### 5. Info Grid

```
┌───────────────────┐ ┌───────────────────┐
│ ⚡                │ │ 💰               │
│ Current Load      │ │ Billing Cycle     │
│ 2.4 kW            │ │ 12 Days Left      │
└───────────────────┘ └───────────────────┘
```

- **Layout**: 2 columns, 16px gap
- **Card Style**: Matches Balance Card
- **Spacing**: 12px gap between icon and text

---

### 6. Action Button

```
┌─────────────────────────────────────────┐
│  📊  View Detailed Statistics         > │
└─────────────────────────────────────────┘
```

- **Background**: #0f1623
- **Border**: 1px #1a2233
- **Border Radius**: 16px
- **Padding**: 20px

---

### 7. Bottom Navigation

```
┌─────────────────────────────────────────┐
│  [🏠]    [📊]    [📡]    [⚙️]         │
│ DASH  STATS  DEVICE  SETTIN            │
└─────────────────────────────────────────┘
```

- **Background**: rgba(15,22,35,0.95)
- **Border Top**: 1px #1a2233
- **Padding**: 12px top, 34px bottom (for home indicator)
- **Active State**: Teal icon + teal text + teal circle background
- **Inactive State**: Gray icon (#94a3b8), gray text

---

## Layout Structure

```markdown
┌─────────────────────────────────────────┐
│ [Status Bar - light-content]            │
├─────────────────────────────────────────┤
│ [Header] ⚡ ENERGY PULSE         [👤]   │
├─────────────────────────────────────────┤
│                                         │
│     [Circular Gauge - 70% width]       │
│          482 kWh + trend                │
│                                         │
├─────────────────────────────────────────┤
│         Estimated Monthly Cost          │
│              1,245.50 EGP               │
│                                         │
├─────────────────────────────────────────┤
│ [Balance Card] 458.00 EGP   [+ TOP UP]  │
├─────────────────────────────────────────┤
│ [Grid] [Current Load: 2.4 kW]           │
│       [Billing: 12 days]                │
├─────────────────────────────────────────┤
│ [Action Button] View Detailed Stats   > │
│                                         │
├─────────────────────────────────────────┤
│ [Nav] DASH | STATS | DEVICES | SETTINGS│
└─────────────────────────────────────────┘
```

---

## Spacing System

```markdown
| Element              | Value  |
|----------------------|--------|
| Screen Padding       | 20-24px|
| Card Padding         | 24px   |
| Card Gap             | 16px   |
| Card Border Radius   | 24px   |
| Button Border Radius | 16px   |
| Section Margin       | 40px   |
| Nav Bar Height       | ~80px  |
```

---

## Dependencies

```json
{
  "dependencies": {
    "react-native-svg": "^15.x",
    "lucide-react-native": "latest",
    "expo-linear-gradient": "~14.0.0"
  }
}
```

---

## Notes

- Pull-to-refresh supported on scroll
- Mock data with random refresh simulation
- Fully functional navigation bar (visual only - routing not implemented)
- Gradient button using expo-linear-gradient
- SVG circle with animated stroke for progress
