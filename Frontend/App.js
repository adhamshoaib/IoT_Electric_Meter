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
import StatCard from './components/statcard';
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
          <View style={styles.loginBackgroundCircleOne} />
          <View style={styles.loginBackgroundCircleTwo} />

          <View style={styles.loginHeader}>
            <View style={styles.logoWrapper}>
              <Text style={styles.logoIcon}>⚡</Text>
            </View>

            <Text style={styles.appTitle}>Smart Meter</Text>
            <Text style={styles.appSubtitle}>
              Simple prepaid energy tracking with a clean modern experience
            </Text>
          </View>

          <View style={styles.loginCard}>
            <Text style={styles.inputLabel}>Email</Text>
            <TextInput
              value={email}
              onChangeText={setEmail}
              placeholder="Enter your email"
              autoCapitalize="none"
              style={styles.input}
              placeholderTextColor="#94a3b8"
            />

            <Text style={styles.inputLabel}>Password</Text>
            <TextInput
              value={password}
              onChangeText={setPassword}
              placeholder="Enter your password"
              secureTextEntry
              style={styles.input}
              placeholderTextColor="#94a3b8"
            />

            <TouchableOpacity
              style={styles.loginButton}
              onPress={() => setIsLoggedIn(true)}
            >
              <Text style={styles.loginButtonText}>Login</Text>
            </TouchableOpacity>

            <Text style={styles.loginFooterText}>
              UI demo version for the smart meter project
            </Text>
          </View>
        </View>
      </SafeAreaView>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      {activeTab === 'dashboard' ? (
        <DashboardScreen user={fakeUser} data={fakeDashboardData} />
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
          <Text style={styles.tabIcon}>⌂</Text>
          <Text
            style={[
              styles.tabText,
              activeTab === 'dashboard' && styles.activeTabText,
            ]}
          >
            Dashboard
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[
            styles.tabButton,
            activeTab === 'profile' && styles.activeTabButton,
          ]}
          onPress={() => setActiveTab('profile')}
        >
          <Text style={styles.tabIcon}>◉</Text>
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

function DashboardScreen({ user, data }) {
  return (
    <ScrollView contentContainerStyle={styles.scrollContent} showsVerticalScrollIndicator={false}>
      <View style={styles.dashboardHeader}>
        <View>
          <Text style={styles.welcomeText}>Welcome back, {user.name}</Text>
          <Text style={styles.headerSubtext}>Your energy at a glance</Text>
        </View>

        <View style={styles.avatarMini}>
          <Text style={styles.avatarMiniText}>{user.name.charAt(0)}</Text>
        </View>
      </View>

      <View style={styles.heroCard}>
        <View style={styles.heroGlowOne} />
        <View style={styles.heroGlowTwo} />

        <Text style={styles.heroSmallLabel}>TODAY’S CONSUMPTION</Text>

        <View style={styles.heroValueRow}>
          <Text style={styles.heroValue}>{data.todayConsumption}</Text>
          <Text style={styles.heroUnit}>kWh</Text>
        </View>

        <Text style={styles.heroNote}>
          This is the main reading the user should understand quickly
        </Text>
      </View>

      <View style={styles.statsRow}>
        <StatCard
          title="Current Balance"
          value={`${data.currentBalance} EGP`}
          subtitle="Prepaid balance"
        />
      </View>

      <View style={styles.statsRowTwo}>
        

        <StatCard
          title="Meter Status"
          value={data.meterStatus}
          subtitle="Smart meter connection"
          status="online"
        />
      </View>

      <View style={styles.warningCard}>
        <View style={styles.warningIconWrap}>
          <Text style={styles.warningIcon}>!</Text>
        </View>

        <View style={styles.warningTextWrap}>
          <Text style={styles.warningTitle}>Low Balance Warning</Text>
          <Text style={styles.warningText}>{data.lowBalanceWarning}</Text>
        </View>
      </View>
    </ScrollView>
  );
}

function ProfileScreen({ user, onLogout }) {
  return (
    <ScrollView contentContainerStyle={styles.scrollContent} showsVerticalScrollIndicator={false}>
      <View style={styles.profileHero}>
        <View style={styles.profileAvatar}>
          <Text style={styles.profileAvatarText}>{user.name.charAt(0)}</Text>
        </View>

        <Text style={styles.profileName}>{user.name}</Text>
        <Text style={styles.profileEmail}>{user.email}</Text>
      </View>

      <View style={styles.profileInfoCard}>
        <Text style={styles.profileSectionTitle}>Account Information</Text>

        <InfoRow label="Meter ID" value={user.meterId} />
        <InfoRow label="Account Number" value={user.accountNumber} />
        <InfoRow label="Address" value={user.address} />
      </View>

      <TouchableOpacity style={styles.logoutButton} onPress={onLogout}>
        <Text style={styles.logoutButtonText}>Logout</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
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
  loginBackgroundCircleOne: {
    position: 'absolute',
    width: 220,
    height: 220,
    borderRadius: 110,
    backgroundColor: '#d1fae5',
    top: -60,
    right: -60,
    opacity: 0.8,
  },
  loginBackgroundCircleTwo: {
    position: 'absolute',
    width: 180,
    height: 180,
    borderRadius: 90,
    backgroundColor: '#e0f2fe',
    bottom: -40,
    left: -40,
    opacity: 0.8,
  },
  loginHeader: {
    alignItems: 'center',
    marginBottom: 28,
  },
  logoWrapper: {
    width: 84,
    height: 84,
    borderRadius: 42,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 18,
    shadowColor: '#0f766e',
    shadowOpacity: 0.25,
    shadowRadius: 18,
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
    lineHeight: 22,
    marginTop: 10,
    paddingHorizontal: 20,
  },
  loginCard: {
    backgroundColor: '#ffffff',
    borderRadius: 28,
    padding: 22,
    shadowColor: '#0f172a',
    shadowOpacity: 0.08,
    shadowRadius: 20,
    shadowOffset: { width: 0, height: 10 },
    elevation: 5,
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
    shadowOpacity: 0.22,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  loginButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '700',
  },
  loginFooterText: {
    textAlign: 'center',
    fontSize: 12,
    color: '#94a3b8',
    marginTop: 16,
  },

  dashboardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  welcomeText: {
    fontSize: 28,
    fontWeight: '800',
    color: '#0f172a',
  },
  headerSubtext: {
    marginTop: 6,
    fontSize: 14,
    color: '#64748b',
  },
  avatarMini: {
    width: 46,
    height: 46,
    borderRadius: 23,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
  },
  avatarMiniText: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '700',
  },

  heroCard: {
    backgroundColor: '#0f766e',
    borderRadius: 30,
    padding: 24,
    marginTop: 20,
    marginBottom: 16,
    overflow: 'hidden',
    shadowColor: '#0f766e',
    shadowOpacity: 0.2,
    shadowRadius: 24,
    shadowOffset: { width: 0, height: 10 },
    elevation: 6,
  },
  heroGlowOne: {
    position: 'absolute',
    width: 180,
    height: 180,
    borderRadius: 90,
    backgroundColor: '#14b8a6',
    top: -50,
    right: -40,
    opacity: 0.22,
  },
  heroGlowTwo: {
    position: 'absolute',
    width: 120,
    height: 120,
    borderRadius: 60,
    backgroundColor: '#99f6e4',
    bottom: -30,
    left: -20,
    opacity: 0.12,
  },
  heroSmallLabel: {
    color: '#ccfbf1',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1,
  },
  heroValueRow: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    marginTop: 18,
  },
  heroValue: {
    color: '#ffffff',
    fontSize: 52,
    fontWeight: '900',
    lineHeight: 58,
  },
  heroUnit: {
    color: '#ccfbf1',
    fontSize: 18,
    fontWeight: '700',
    marginLeft: 8,
    marginBottom: 8,
  },
  heroNote: {
    color: '#d1fae5',
    fontSize: 13,
    lineHeight: 20,
    marginTop: 14,
    maxWidth: '90%',
  },

  statsRow: {
    marginBottom: 14,
  },
  statsRowTwo: {
    flexDirection: 'row',
    gap: 14,
    marginBottom: 16,
  },

  warningCard: {
    backgroundColor: '#fff7ed',
    borderRadius: 22,
    borderWidth: 1,
    borderColor: '#fed7aa',
    padding: 16,
    flexDirection: 'row',
    alignItems: 'flex-start',
  },
  warningIconWrap: {
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
  warningTextWrap: {
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
    lineHeight: 21,
    color: '#9a3412',
  },

  profileHero: {
    backgroundColor: '#ffffff',
    borderRadius: 30,
    paddingVertical: 28,
    paddingHorizontal: 20,
    alignItems: 'center',
    marginBottom: 16,
    shadowColor: '#0f172a',
    shadowOpacity: 0.06,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
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
    fontSize: 34,
    fontWeight: '800',
  },
  profileName: {
    fontSize: 28,
    fontWeight: '800',
    color: '#0f172a',
  },
  profileEmail: {
    marginTop: 6,
    fontSize: 14,
    color: '#64748b',
  },
  profileInfoCard: {
    backgroundColor: '#ffffff',
    borderRadius: 26,
    padding: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.06,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  profileSectionTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 8,
  },
  infoRow: {
    paddingVertical: 14,
    borderBottomWidth: 1,
    borderBottomColor: '#f1f5f9',
  },
  infoLabel: {
    fontSize: 12,
    fontWeight: '700',
    color: '#94a3b8',
    textTransform: 'uppercase',
    marginBottom: 6,
  },
  infoValue: {
    fontSize: 15,
    color: '#0f172a',
    fontWeight: '600',
  },
  logoutButton: {
    backgroundColor: '#ef4444',
    borderRadius: 18,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 20,
    shadowColor: '#ef4444',
    shadowOpacity: 0.18,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
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
    shadowRadius: 20,
    shadowOffset: { width: 0, height: 8 },
    elevation: 6,
  },
  tabButton: {
    flex: 1,
    borderRadius: 16,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
  },
  activeTabButton: {
    backgroundColor: '#ecfdf5',
  },
  tabIcon: {
    fontSize: 14,
    color: '#64748b',
    marginBottom: 4,
  },
  tabText: {
    fontSize: 13,
    fontWeight: '700',
    color: '#64748b',
  },
  activeTabText: {
    color: '#0f766e',
  },
});