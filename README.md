# IoT Electric Meter 📊⚡

An intelligent IoT-based electric metering system that combines hardware, backend, and mobile frontend components to provide real-time energy consumption monitoring, billing management, and top-up capabilities.

## 🎯 Overview

This project implements a complete smart metering solution using:
- **ESP32 Microcontroller** with BL0939 energy measurement IC
- **React Native Mobile App** for user interface and billing
- **Node.js Backend** for data aggregation and billing calculation
- **Firebase** for authentication and real-time data management
- **GSM/WiFi Connectivity** for reliable network access

The system enables users to monitor electricity consumption in real-time and manage payments through multiple channels including mobile wallet, credit card, and bank transfer.

---

## 🏗️ Project Structure

```
IoT_Electric_Meter/
├── ESP32-firmware/          # ESP32 embedded firmware (C)
│   ├── main/                # Main application entry point
│   ├── components/          # Modular firmware components
│   │   ├── BL0939/          # Energy measurement IC driver
│   │   ├── uart_service/    # UART communication
│   │   ├── i2c_service/     # I2C protocol handler
│   │   ├── oled_display/    # OLED display driver
│   │   ├── wifi_sta/        # WiFi connectivity
│   │   ├── gsm_driver/      # GSM/cellular connectivity
│   │   ├── led_driver/      # LED status indicators
│   │   ├── cloud_sync/      # Cloud data synchronization
│   │   ├── http_client/     # HTTP request handling
│   │   └── energy_metering/ # Energy calculation utilities
│   └── CMakeLists.txt       # Build configuration
│
├── Frontend/                # React Native mobile app (JavaScript)
│   ├── App.js              # Main application component
│   ├── screens/            # Screen components
│   │   ├── DashboardScreen # Energy consumption dashboard
│   │   ├── BillingScreen   # Billing and payment history
│   │   ├── SettingsScreen  # User preferences
│   │   ├── ProfileScreen   # User profile management
│   │   ├── StatisticsScreen# Energy statistics and trends
│   │   └── payments/       # Payment method screens
│   ├── components/         # Reusable UI components
│   ├── services/           # Business logic and utilities
│   │   ├── firebase.js     # Firebase configuration
│   │   └── calculateEgyptBill.js # Egyptian billing calculation
│   └── styles/             # Styling configuration
│
├── backend/                 # Node.js backend server (JavaScript)
│   ├── server.js           # Main server application
│   ├── simulate_esp32.js   # ESP32 simulator for testing
│   └── ...                 # Additional backend services
│
└── README.md              # This file

```

---

## 🔧 Hardware Components

- **ESP32 Microcontroller** - Main processing unit
- **BL0939 Energy IC** - Accurate power measurement (voltage, current, energy)
- **OLED Display** - Real-time consumption display
- **GSM Module** - Cellular fallback connectivity
- **WiFi Module** - Primary network connectivity
- **LED Indicators** - System status visualization
- **I2C/UART Interfaces** - Device communication

---

## 💻 Technology Stack

| Component | Technology | Language |
|-----------|-----------|----------|
| **Firmware** | ESP-IDF, FreeRTOS | C (60.3%) |
| **Frontend** | React Native, Expo, Firebase | JavaScript (38.9%) |
| **Backend** | Node.js, Express, Firebase | JavaScript |
| **Database** | Firebase Realtime Database, Firebase Auth | - |
| **Storage** | AsyncStorage (mobile) | - |

---

## 🚀 Getting Started

### Prerequisites

- **For ESP32 Firmware:**
  - ESP-IDF v5.0 or higher
  - ESP32 DevKit
  - Python 3.8+
  - GCC compiler toolchain

- **For Frontend:**
  - Node.js 16+
  - npm or yarn
  - Expo CLI
  - Mobile device (Android/iOS)

- **For Backend:**
  - Node.js 16+
  - npm
  - Firebase account

### Installation

#### 1. **ESP32 Firmware Setup**

```bash
# Clone and navigate to ESP32 firmware directory
cd ESP32-firmware

# Configure the project
idf.py set-target esp32
idf.py menuconfig

# Build and flash to ESP32
idf.py build
idf.py -p /dev/ttyUSB0 flash
```

#### 2. **Frontend Setup**

```bash
# Navigate to frontend directory
cd Frontend

# Install dependencies
npm install

# Start Expo development server
npx expo start

# Scan QR code with Expo Go app on your mobile device
```

#### 3. **Backend Setup**

```bash
# Navigate to backend directory
cd backend

# Install dependencies
npm install

# Set environment variables
export SERVER_URL="http://localhost:3000"
export ESP32_API_KEY="sem-dev-key-2026"
export FIREBASE_CONFIG="..."

# Start the server
npm start
```

---

## 📱 Features

### Mobile App
- ✅ **Real-time Dashboard** - Live consumption, monthly totals, current balance
- ✅ **Billing Management** - View bills, payment history, and usage trends
- ✅ **Multiple Payment Methods** - Mobile wallet, credit card, bank transfer
- ✅ **Authentication** - Secure Firebase login/registration
- ✅ **User Profile** - Personal information and meter details
- ✅ **Statistics** - Detailed consumption graphs and analysis

### ESP32 Firmware
- ✅ **Energy Measurement** - Accurate power consumption tracking via BL0939
- ✅ **WiFi/GSM Connectivity** - Dual connectivity with automatic fallback
- ✅ **OLED Display** - Real-time consumption display
- ✅ **Cloud Sync** - Automatic data upload to backend
- ✅ **LED Status Indicators** - System health visualization
- ✅ **NTP Time Synchronization** - Accurate timestamp management

### Backend
- ✅ **Data Aggregation** - Collect and store meter readings
- ✅ **Billing Calculation** - Egyptian tariff-based bill computation
- ✅ **API Endpoints** - RESTful API for mobile app
- ✅ **Authentication** - Firebase integration for user management
- ✅ **Testing Support** - ESP32 simulator for development

---

## 🔐 Configuration

### Firebase Setup

1. Create a Firebase project at [firebase.google.com](https://firebase.google.com)
2. Enable Authentication (Email/Password)
3. Create a Realtime Database
4. Download service account key
5. Configure in frontend (`Frontend/services/firebase.js`)

### ESP32 Configuration

Edit `ESP32-firmware/components/cloud_sync/Kconfig.projbuild` to customize:
- Upload interval
- Retry delay
- NTP servers
- GSM APN settings

### Backend Configuration

Set environment variables:
```bash
SERVER_URL=https://your-server.com
ESP32_API_KEY=your-secure-api-key
FIREBASE_DATABASE_URL=your-firebase-db-url
```

---

## 🧪 Testing

### ESP32 Firmware
```bash
# Flash firmware and monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

### Backend with Simulator
```bash
# Start the ESP32 simulator to send fake readings
node backend/simulate_esp32.js

# In another terminal, start the server
npm start
```

### Mobile App
- Use Expo Go app for quick testing
- Build APK/IPA for production deployment

---

## 📊 API Endpoints

### Reading Submission
```
POST /reading
Headers: x-api-key: <API_KEY>
Body: { energy_kwh: float, ts: timestamp }
```

### User Readings
```
GET /users/:userId/readings
Response: Array of readings with timestamps
```

### Billing
```
GET /users/:userId/billing
Response: Current bill, payment history, balance
```

---

## 🛠️ Development Workflow

1. **Firmware Updates** → Flash to ESP32 → Test with OLED display
2. **Backend Changes** → Run simulator → Verify API responses
3. **Frontend Updates** → Hot reload via Expo → Test on device
4. **Integration** → Deploy backend → Update app configuration → Test end-to-end

---

## 📝 Key Components Overview

### Energy Measurement (BL0939)
- Measures voltage, current, and power consumption
- Communicates via UART (4800 baud)
- Calibration support for accuracy tuning

### Cloud Sync
- Uploads readings at configurable intervals (default: 30s)
- Automatic retry with exponential backoff
- NTP-based time synchronization
- WiFi primary, GSM fallback

### Mobile UI
- Built with React Native for cross-platform support
- Firebase real-time database synchronization
- Egyptian electricity tariff billing calculations
- Multiple payment method support

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| ESP32 won't flash | Check USB cable, verify COM port, ensure ESP-IDF is installed |
| No WiFi connection | Verify SSID/password in WiFi config, check WiFi range |
| Firebase auth fails | Verify Firebase config, check internet connection |
| Inaccurate readings | Calibrate BL0939 using cloud_sync configuration |
| App crashes on startup | Clear cache, reinstall dependencies, check Firebase config |

---

## 📈 Performance Metrics

- **Energy Measurement Accuracy**: ±2% (depends on BL0939 calibration)
- **Upload Interval**: 30 seconds (configurable)
- **Display Refresh**: ~1 second
- **WiFi Connection Time**: 5-10 seconds
- **GSM Fallback Time**: 10-30 seconds

---

## 📄 License

This project is provided as-is for educational and personal use.

---

## 👤 Author

**Adham Shoaib**
- GitHub: [@adhamshoaib](https://github.com/adhamshoaib)

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/adhamshoaib/IoT_Electric_Meter/issues).

---

## 📞 Support

For questions or issues:
1. Check existing [GitHub Issues](https://github.com/adhamshoaib/IoT_Electric_Meter/issues)
2. Review relevant documentation in the component directories
3. Test with the backend simulator (`simulate_esp32.js`)

---

## 🔮 Future Enhancements

- [ ] Web dashboard for consumption analytics
- [ ] SMS billing notifications
- [ ] Multiple meter support per user
- [ ] Predictive consumption analysis
- [ ] Energy saving recommendations
- [ ] Integration with utility provider APIs
- [ ] LoRaWAN support for wider coverage
- [ ] Machine learning for anomaly detection

---

**Last Updated:** June 2026  
**Status:** Active Development
