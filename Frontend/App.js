import React, { useState } from 'react';

import {
  SafeAreaView,
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  ScrollView,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { fakeUser, fakeDashboardData } from './services/fakedata';

export default function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [activeTab, setActiveTab] = useState('dashboard');
  const [email, setEmail] = useState('omar@example.com');
  const [password, setPassword] = useState('123456');

  if (!isLoggedIn) {
    return (
      <SafeAreaView style={styles.container}>
        <View style={styles.loginScreen}>
          <View style={styles.loginGlowTop} />
          <View style={styles.loginGlowBottom} />

          <View style={styles.logoCircle}>
            <Text style={styles.logoIcon}>⚡</Text>
          </View>

          <Text style={styles.appTitle}>Smart Meter</Text>
          <Text style={styles.appSubtitle}>
            Track your electricity usage in a simple and clear way
          </Text>

          <View style={styles.loginCard}>
            <Text style={styles.inputLabel}>Email</Text>
            <TextInput
              value={email}
              onChangeText={setEmail}
              placeholder="Enter your email"
              autoCapitalize="none"
              placeholderTextColor="#94a3b8"
              style={styles.input}
            />

            <Text style={styles.inputLabel}>Password</Text>
            <TextInput
              value={password}
              onChangeText={setPassword}
              placeholder="Enter your password"
              secureTextEntry
              placeholderTextColor="#94a3b8"
              style={styles.input}
            />

            <TouchableOpacity
              style={styles.loginButton}
              onPress={() => setIsLoggedIn(true)}
            >
              <Text style={styles.loginButtonText}>Login</Text>
            </TouchableOpacity>
          </View>
        </View>
      </SafeAreaView>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
     {activeTab === 'dashboard' ? (
  <DashboardScreen
    user={fakeUser}
    data={fakeDashboardData}
    onOpenProfile={() => setActiveTab('profile')}
  />
) : (
  <ProfileScreen
    user={fakeUser}
    onLogout={() => {
      setIsLoggedIn(false);
      setActiveTab('dashboard');
    }}
  />
)}
<View style={styles.tabBar}>
  <TouchableOpacity
    style={[
      styles.tabButton,
      activeTab === 'dashboard' && styles.activeTabButton,
    ]}
    onPress={() => setActiveTab('dashboard')}
  >
    <Ionicons
      name="home"
      size={20}
      color={activeTab === 'dashboard' ? '#0f766e' : '#64748b'}
    />
    <Text
      style={[
        styles.tabText,
        activeTab === 'dashboard' && styles.activeTabText,
      ]}
    >
      Home
    </Text>
  </TouchableOpacity>

  <TouchableOpacity
    style={[
      styles.tabButton,
      activeTab === 'profile' && styles.activeTabButton,
    ]}
    onPress={() => setActiveTab('profile')}
  >
    <Ionicons
      name="person"
      size={20}
      color={activeTab === 'profile' ? '#0f766e' : '#64748b'}
    />
    <Text
      style={[
        styles.tabText,
        activeTab === 'profile' && styles.activeTabText,
      ]}
    >
      Profile
    </Text>
  </TouchableOpacity>
</View>
    </SafeAreaView>
  );
}
function DashboardScreen({ user, data, onOpenProfile  }) {
  const consumptionValue =
    data.monthlyConsumption ?? data.todayConsumption ?? 412;

  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.headerRow}>
        <View>
          <Text style={styles.welcomeText}>Welcome back, {user.name}</Text>
          <Text style={styles.headerSubtitle}>
            Your prepaid smart meter overview
          </Text>
        </View>

        <TouchableOpacity style={styles.headerAvatar} onPress={onOpenProfile}>
  <Text style={styles.headerAvatarText}>{user.name.charAt(0)}</Text>
</TouchableOpacity>
      </View>

      <View style={styles.heroCard}>
        <View style={styles.heroGlowOne} />
        <View style={styles.heroGlowTwo} />

        <View style={styles.heroBadge}>
          <Text style={styles.heroBadgeText}>THIS MONTH</Text>
        </View>

        <Text style={styles.heroLabel}>Monthly Consumption</Text>

        <View style={styles.heroValueRow}>
          <Text style={styles.heroValue}>{consumptionValue}</Text>
          <Text style={styles.heroUnit}>kWh</Text>
        </View>

        <Text style={styles.heroNote}>
          Your total electricity consumption for the current month
        </Text>
      </View>

      <View style={styles.balanceSection}>
        <View>
          <Text style={styles.balanceSectionLabel}>Current Balance</Text>
          <Text style={styles.balanceSectionValue}>{data.currentBalance} EGP</Text>
        </View>

        <View style={styles.balanceMiniPill}>
          <Text style={styles.balanceMiniPillText}>Prepaid Meter</Text>
        </View>
      </View>

      <View style={styles.divider} />

      <View style={styles.statusSection}>
        <View style={styles.statusIconArea}>
          <View style={styles.signalDotOuter}>
            <View style={styles.signalDotInner} />
          </View>

          <View style={styles.barsWrap}>
            <View style={[styles.bar, styles.barOne]} />
            <View style={[styles.bar, styles.barTwo]} />
            <View style={[styles.bar, styles.barThree]} />
            <View style={[styles.bar, styles.barFour]} />
          </View>
        </View>

        <View style={styles.statusTextWrap}>
          <Text style={styles.statusEyebrow}>METER STATUS</Text>
          <Text style={styles.statusTitle}>Connection Stable</Text>
          <Text style={styles.statusSubtitle}>
            Receiving readings normally
          </Text>
        </View>

        <View style={styles.syncPill}>
          <Text style={styles.syncPillText}>
            {data.lastSync ?? '2 min ago'}
          </Text>
        </View>
      </View>

      <View style={styles.warningBanner}>
        <View style={styles.warningIconCircle}>
          <Text style={styles.warningIcon}>!</Text>
        </View>

        <View style={styles.warningContent}>
          <Text style={styles.warningTitle}>Low Balance Warning</Text>
          <Text style={styles.warningText}>{data.lowBalanceWarning}</Text>
        </View>
      </View>
    </ScrollView>
  );
}

function ProfileScreen({ user, onLogout }) {
  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.profileHero}>
        <View style={styles.profileAvatar}>
          <Text style={styles.profileAvatarText}>{user.name.charAt(0)}</Text>
        </View>

        <Text style={styles.profileName}>{user.name}</Text>
        <Text style={styles.profileEmail}>{user.email}</Text>
      </View>

      <View style={styles.profileCard}>
        <Text style={styles.sectionTitle}>Account Info</Text>

        <ProfileRow label="Meter ID" value={user.meterId} />
        <ProfileRow label="Account Number" value={user.accountNumber} />
        <ProfileRow label="Address" value={user.address} lastItem />
      </View>

      <TouchableOpacity style={styles.logoutButton} onPress={onLogout}>
        <Text style={styles.logoutButtonText}>Logout</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

function ProfileRow({ label, value, lastItem }) {
  return (
    <View style={[styles.profileRow, lastItem && styles.profileRowLast]}>
      <Text style={styles.profileRowLabel}>{label}</Text>
      <Text style={styles.profileRowValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f4f7fb',
  },

  scrollContent: {
    padding: 20,
    paddingBottom: 110,
  },

  loginScreen: {
    flex: 1,
    justifyContent: 'center',
    padding: 24,
    overflow: 'hidden',
  },
  loginGlowTop: {
    position: 'absolute',
    top: -60,
    right: -40,
    width: 180,
    height: 180,
    borderRadius: 90,
    backgroundColor: '#d1fae5',
    opacity: 0.8,
  },
  loginGlowBottom: {
    position: 'absolute',
    bottom: -50,
    left: -40,
    width: 160,
    height: 160,
    borderRadius: 80,
    backgroundColor: '#dbeafe',
    opacity: 0.8,
  },
  logoCircle: {
    width: 82,
    height: 82,
    borderRadius: 41,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
    alignSelf: 'center',
    marginBottom: 18,
    shadowColor: '#0f766e',
    shadowOpacity: 0.22,
    shadowRadius: 16,
    shadowOffset: { width: 0, height: 8 },
    elevation: 5,
  },
  logoIcon: {
    fontSize: 34,
    color: '#ffffff',
  },
  appTitle: {
    fontSize: 32,
    fontWeight: '800',
    color: '#0f172a',
    textAlign: 'center',
  },
  appSubtitle: {
    fontSize: 14,
    color: '#64748b',
    textAlign: 'center',
    marginTop: 8,
    marginBottom: 28,
    lineHeight: 21,
  },
  loginCard: {
    backgroundColor: '#ffffff',
    borderRadius: 26,
    padding: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.06,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  inputLabel: {
    fontSize: 14,
    color: '#334155',
    fontWeight: '600',
    marginBottom: 8,
    marginTop: 12,
  },
  input: {
    backgroundColor: '#f8fafc',
    borderWidth: 1,
    borderColor: '#e2e8f0',
    borderRadius: 16,
    paddingHorizontal: 16,
    paddingVertical: 14,
    fontSize: 15,
    color: '#0f172a',
  },
  loginButton: {
    backgroundColor: '#0f766e',
    borderRadius: 16,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 24,
    shadowColor: '#0f766e',
    shadowOpacity: 0.2,
    shadowRadius: 12,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  loginButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '700',
  },

headerRow: {
  flexDirection: 'row',
  justifyContent: 'space-between',
  alignItems: 'center',
},
welcomeText: {
  fontSize: 28,
  fontWeight: '800',
  color: '#0f172a',
},
headerSubtitle: {
  fontSize: 14,
  color: '#64748b',
  marginTop: 6,
},
headerAvatar: {
  width: 46,
  height: 46,
  borderRadius: 23,
  backgroundColor: '#0f766e',
  alignItems: 'center',
  justifyContent: 'center',
},
headerAvatarText: {
  color: '#ffffff',
  fontSize: 18,
  fontWeight: '800',
},

heroCard: {
  backgroundColor: '#0f766e',
  borderRadius: 30,
  padding: 24,
  marginTop: 22,
  marginBottom: 22,
  overflow: 'hidden',
  shadowColor: '#0f766e',
  shadowOpacity: 0.18,
  shadowRadius: 20,
  shadowOffset: { width: 0, height: 10 },
  elevation: 6,
},
heroGlowOne: {
  position: 'absolute',
  width: 170,
  height: 170,
  borderRadius: 85,
  backgroundColor: '#14b8a6',
  top: -50,
  right: -30,
  opacity: 0.18,
},
heroGlowTwo: {
  position: 'absolute',
  width: 110,
  height: 110,
  borderRadius: 55,
  backgroundColor: '#99f6e4',
  bottom: -25,
  left: -15,
  opacity: 0.12,
},
heroBadge: {
  alignSelf: 'flex-start',
  backgroundColor: 'rgba(255,255,255,0.16)',
  paddingHorizontal: 12,
  paddingVertical: 6,
  borderRadius: 999,
},
heroBadgeText: {
  color: '#ffffff',
  fontSize: 11,
  fontWeight: '800',
  letterSpacing: 0.8,
},
heroLabel: {
  color: '#d1fae5',
  fontSize: 16,
  fontWeight: '600',
  marginTop: 18,
  marginBottom: 12,
},
heroValueRow: {
  flexDirection: 'row',
  alignItems: 'flex-end',
},
heroValue: {
  color: '#ffffff',
  fontSize: 58,
  fontWeight: '900',
  lineHeight: 62,
},
heroUnit: {
  color: '#d1fae5',
  fontSize: 21,
  fontWeight: '700',
  marginLeft: 8,
  marginBottom: 8,
},
heroNote: {
  color: '#ccfbf1',
  fontSize: 13,
  marginTop: 14,
  lineHeight: 20,
  maxWidth: '90%',
},

balanceSection: {
  flexDirection: 'row',
  justifyContent: 'space-between',
  alignItems: 'flex-start',
  paddingTop: 6,
  paddingBottom: 18,
},
balanceSectionLabel: {
  fontSize: 14,
  color: '#64748b',
  marginBottom: 8,
},
balanceSectionValue: {
  fontSize: 34,
  fontWeight: '800',
  color: '#0f766e',
},
balanceMiniPill: {
  backgroundColor: '#e6fffa',
  paddingHorizontal: 12,
  paddingVertical: 8,
  borderRadius: 999,
},
balanceMiniPillText: {
  fontSize: 12,
  fontWeight: '700',
  color: '#0f766e',
},

divider: {
  height: 1,
  backgroundColor: '#e2e8f0',
  marginBottom: 20,
},

statusSection: {
  flexDirection: 'row',
  alignItems: 'center',
  paddingBottom: 22,
},
statusIconArea: {
  width: 72,
  alignItems: 'center',
  marginRight: 14,
},
signalDotOuter: {
  width: 30,
  height: 30,
  borderRadius: 15,
  backgroundColor: '#dcfce7',
  alignItems: 'center',
  justifyContent: 'center',
  marginBottom: 10,
},
signalDotInner: {
  width: 12,
  height: 12,
  borderRadius: 6,
  backgroundColor: '#22c55e',
},
barsWrap: {
  flexDirection: 'row',
  alignItems: 'flex-end',
  height: 20,
},
bar: {
  width: 5,
  backgroundColor: '#22c55e',
  borderRadius: 3,
  marginHorizontal: 2,
},
barOne: {
  height: 6,
},
barTwo: {
  height: 10,
},
barThree: {
  height: 14,
},
barFour: {
  height: 18,
},
statusTextWrap: {
  flex: 1,
},
statusEyebrow: {
  fontSize: 11,
  fontWeight: '800',
  letterSpacing: 0.8,
  color: '#94a3b8',
  marginBottom: 6,
},
statusTitle: {
  fontSize: 22,
  fontWeight: '800',
  color: '#0f172a',
  marginBottom: 4,
},
statusSubtitle: {
  fontSize: 14,
  color: '#64748b',
  lineHeight: 20,
},
syncPill: {
  backgroundColor: '#ecfdf5',
  paddingHorizontal: 12,
  paddingVertical: 8,
  borderRadius: 999,
  marginLeft: 10,
},
syncPillText: {
  color: '#0f766e',
  fontSize: 12,
  fontWeight: '700',
},

warningBanner: {
  backgroundColor: '#fff7ed',
  borderRadius: 22,
  padding: 16,
  borderWidth: 1,
  borderColor: '#fed7aa',
  flexDirection: 'row',
  alignItems: 'flex-start',
},
warningIconCircle: {
  width: 34,
  height: 34,
  borderRadius: 17,
  backgroundColor: '#fb923c',
  alignItems: 'center',
  justifyContent: 'center',
  marginRight: 12,
},
warningIcon: {
  color: '#ffffff',
  fontSize: 18,
  fontWeight: '800',
},
warningContent: {
  flex: 1,
},
warningTitle: {
  fontSize: 15,
  fontWeight: '800',
  color: '#c2410c',
  marginBottom: 6,
},
warningText: {
  fontSize: 14,
  lineHeight: 20,
  color: '#9a3412',
},
  profileHero: {
    backgroundColor: '#ffffff',
    borderRadius: 28,
    paddingVertical: 28,
    paddingHorizontal: 20,
    alignItems: 'center',
    marginBottom: 16,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  profileAvatar: {
    width: 92,
    height: 92,
    borderRadius: 46,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 14,
  },
  profileAvatarText: {
    color: '#ffffff',
    fontSize: 36,
    fontWeight: '800',
  },
  profileName: {
    fontSize: 28,
    fontWeight: '800',
    color: '#0f172a',
  },
  profileEmail: {
    fontSize: 14,
    color: '#64748b',
    marginTop: 6,
  },
  profileCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  profileRow: {
    paddingVertical: 18,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  profileRowLast: {
    borderBottomWidth: 0,
  },
  profileRowLabel: {
    fontSize: 12,
    color: '#94a3b8',
    textTransform: 'uppercase',
    fontWeight: '700',
    marginBottom: 6,
  },
  profileRowValue: {
    fontSize: 16,
    color: '#0f172a',
    fontWeight: '600',
  },
  logoutButton: {
    backgroundColor: '#ef4444',
    borderRadius: 16,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 20,
  },
  logoutButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '800',
  },

  tabBar: {
    position: 'absolute',
    left: 16,
    right: 16,
    bottom: 16,
    flexDirection: 'row',
    backgroundColor: '#ffffff',
    borderRadius: 22,
    padding: 8,
    shadowColor: '#0f172a',
    shadowOpacity: 0.08,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 6 },
    elevation: 5,
  },
  tabButton: {
    flex: 1,
    borderRadius: 14,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
  },
  activeTabButton: {
    backgroundColor: '#ecfdf5',
  },
  tabText: {
    fontSize: 14,
    fontWeight: '700',
    color: '#64748b',
  },
  activeTabText: {
    color: '#0f766e',
  },
});