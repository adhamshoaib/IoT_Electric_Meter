import React, { useEffect, useState } from 'react';
import { ref, onValue } from 'firebase/database';
import { database } from './services/firebase';
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
import StatisticsScreen from './screens/StatisticsScreen';

export default function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [activeScreen, setActiveScreen] = useState('dashboard');
  const [email, setEmail] = useState('omar@example.com');
  const [password, setPassword] = useState('123456');
  const [liveDashboardData, setLiveDashboardData] = useState(fakeDashboardData);
useEffect(() => {
  const meterRef = ref(database, 'current_reading');

  const unsubscribe = onValue(meterRef, (snapshot) => {
    const firebaseData = snapshot.val();

    if (firebaseData) {
      setLiveDashboardData((prev) => ({
        ...prev,
        monthlyConsumption: firebaseData.energy_kwh ?? prev.monthlyConsumption,
        lastSync: firebaseData.ts ? String(firebaseData.ts) : 'Just now',
      }));
    }
  });

  return () => unsubscribe();
}, []);

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
              onPress={() => {
                setIsLoggedIn(true);
                setActiveScreen('dashboard');
              }}
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
      {activeScreen === 'dashboard' && (
        <DashboardScreen
          user={fakeUser}
          data={liveDashboardData}
          onOpenSettings={() => setActiveScreen('settings')}
        />
      )}

      {activeScreen === 'settings' && (
        <SettingsScreen
          onBack={() => setActiveScreen('dashboard')}
          onOpenProfile={() => setActiveScreen('profile')}
          onOpenBilling={() => setActiveScreen('billing')}
          onLogout={() => {
            setIsLoggedIn(false);
            setActiveScreen('dashboard');
          }}
        />
      )}

      {activeScreen === 'profile' && (
        <ProfileScreen
          user={fakeUser}
          onBack={() => setActiveScreen('settings')}
        />
      )}

      {activeScreen === 'statistics' && (
        <StatisticsScreen onBack={() => setActiveScreen('dashboard')} />
      )}

      {activeScreen === 'billing' && (
        <BillingScreen
          data={liveDashboardData}
          onBack={() => setActiveScreen('settings')}
        />
      )}

      <View style={styles.tabBar}>
        <TouchableOpacity
          style={[
            styles.tabButton,
            activeScreen === 'dashboard' && styles.activeTabButton,
          ]}
          onPress={() => setActiveScreen('dashboard')}
        >
          <Ionicons
            name="home"
            size={20}
            color={activeScreen === 'dashboard' ? '#0f766e' : '#64748b'}
          />
          <Text
            style={[
              styles.tabText,
              activeScreen === 'dashboard' && styles.activeTabText,
            ]}
          >
            Home
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[
            styles.tabButton,
            activeScreen === 'statistics' && styles.activeTabButton,
          ]}
          onPress={() => setActiveScreen('statistics')}
        >
          <Ionicons
            name="stats-chart"
            size={20}
            color={activeScreen === 'statistics' ? '#0f766e' : '#64748b'}
          />
          <Text
            style={[
              styles.tabText,
              activeScreen === 'statistics' && styles.activeTabText,
            ]}
          >
            Stats
          </Text>
        </TouchableOpacity>
      </View>
    </SafeAreaView>
  );
}

function DashboardScreen({ user, data, onOpenSettings }) {
  const consumptionValue =
    data.monthlyConsumption ?? data.todayConsumption ?? 412;

  const estimatedCost = data.estimatedCost ?? 238;
  const lastTopUpAmount = data.lastTopUp?.amount ?? 200;
  const lastTopUpDate = data.lastTopUp?.date ?? '10 Apr 2026';

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

        <TouchableOpacity style={styles.headerAvatar} onPress={onOpenSettings}>
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

        <Text style={styles.heroNote}>Updated from meter reading</Text>

        <Text style={styles.heroMetaText}>
          Last sync: {data.lastSync ?? 'Just now'}
        </Text>
      </View>

      <View style={styles.balanceSection}>
        <View>
          <Text style={styles.balanceSectionLabel}>Current Balance</Text>
          <Text style={styles.balanceSectionValue}>
            {data.currentBalance} EGP
          </Text>
        </View>

        <View style={styles.balanceMiniPill}>
          <Text style={styles.balanceMiniPillText}>Prepaid Meter</Text>
        </View>
      </View>

      <View style={styles.billingPreviewCard}>
        <View style={styles.billingPreviewHeader}>
          <Text style={styles.billingPreviewTitle}>Billing Snapshot</Text>
          <Text style={styles.billingPreviewSubtitle}>Quick summary</Text>
        </View>

        <View style={styles.billingPreviewRow}>
          <View style={styles.billingPreviewItem}>
            <Text style={styles.billingPreviewItemLabel}>Estimated Cost</Text>
            <Text style={styles.billingPreviewItemValue}>
              {estimatedCost} EGP
            </Text>
          </View>

          <View style={styles.billingPreviewItem}>
            <Text style={styles.billingPreviewItemLabel}>Last Top-Up</Text>
            <Text style={styles.billingPreviewItemValue}>
              {lastTopUpAmount} EGP
            </Text>
            <Text style={styles.billingPreviewItemDate}>{lastTopUpDate}</Text>
          </View>
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

function SettingsScreen({ onBack, onOpenProfile, onOpenBilling, onLogout }) {
  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Settings</Text>

        <View style={styles.topSpacer} />
      </View>

      <View style={styles.sectionCard}>
        <SettingsRow
          icon="person-outline"
          title="Profile"
          subtitle="View account and meter details"
          onPress={onOpenProfile}
        />

        <SettingsRow
          icon="card-outline"
          title="Billing"
          subtitle="View payments and monthly estimate"
          onPress={onOpenBilling}
          lastItem
        />
      </View>

      <TouchableOpacity style={styles.logoutButton} onPress={onLogout}>
        <Text style={styles.logoutButtonText}>Logout</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

function SettingsRow({ icon, title, subtitle, onPress, lastItem }) {
  return (
    <TouchableOpacity
      style={[styles.settingsRow, lastItem && styles.settingsRowLast]}
      onPress={onPress}
      activeOpacity={0.8}
    >
      <View style={styles.settingsRowLeft}>
        <View style={styles.settingsIconWrap}>
          <Ionicons name={icon} size={20} color="#0f766e" />
        </View>

        <View style={styles.settingsTextWrap}>
          <Text style={styles.settingsRowTitle}>{title}</Text>
          <Text style={styles.settingsRowSubtitle}>{subtitle}</Text>
        </View>
      </View>

      <Ionicons name="chevron-forward" size={18} color="#94a3b8" />
    </TouchableOpacity>
  );
}

function ProfileScreen({ user, onBack }) {
  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Profile</Text>

        <View style={styles.topSpacer} />
      </View>

      <View style={styles.profileHero}>
        <View style={styles.profileAvatar}>
          <Text style={styles.profileAvatarText}>{user.name.charAt(0)}</Text>
        </View>

        <Text style={styles.profileName}>{user.name}</Text>
        <Text style={styles.profileEmail}>{user.email}</Text>
      </View>

      <View style={styles.sectionCard}>
        <Text style={styles.sectionTitle}>Account Info</Text>

        <ProfileRow label="Meter ID" value={user.meterId} />
        <ProfileRow label="Account Number" value={user.accountNumber} />
        <ProfileRow label="Address" value={user.address} lastItem />
      </View>
    </ScrollView>
  );
}

function BillingScreen({ data, onBack }) {
  const estimatedCost = data.estimatedCost ?? 238;
  const currentBalance = data.currentBalance ?? 120;
  const lastTopUpAmount = data.lastTopUp?.amount ?? 200;
  const lastTopUpDate = data.lastTopUp?.date ?? '10 Apr 2026';

  const paymentHistory = data.paymentHistory ?? [
    { id: '1', date: '10 Apr 2026', method: 'Bank Card', amount: 200 },
    { id: '2', date: '02 Apr 2026', method: 'Mobile Wallet', amount: 150 },
    { id: '3', date: '25 Mar 2026', method: 'Cash Payment', amount: 100 },
  ];

  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Billing</Text>

        <View style={styles.topSpacer} />
      </View>

      <View style={styles.billingSummaryCard}>
        <Text style={styles.billingEyebrow}>THIS MONTH</Text>
        <Text style={styles.billingMainValue}>{estimatedCost} EGP</Text>
        <Text style={styles.billingSummaryText}>
          Estimated cost based on current monthly consumption
        </Text>

        <View style={styles.billingStatsRow}>
          <View style={styles.billingStatItem}>
            <Text style={styles.billingStatLabel}>Current Balance</Text>
            <Text style={styles.billingStatValue}>{currentBalance} EGP</Text>
          </View>

          <View style={styles.billingStatItem}>
            <Text style={styles.billingStatLabel}>Last Top-Up</Text>
            <Text style={styles.billingStatValue}>{lastTopUpAmount} EGP</Text>
            <Text style={styles.billingStatSubtext}>{lastTopUpDate}</Text>
          </View>
        </View>
      </View>

      <View style={[styles.sectionCard, styles.sectionCardSpacing]}>
        <Text style={styles.sectionTitle}>Payment Methods</Text>
        <Text style={styles.billingSectionHint}>
          Choose how you would like to pay or recharge your balance
        </Text>

        <PaymentMethodCard
          icon="card-outline"
          title="Bank Card"
          subtitle="Pay using your debit or credit card"
          tag="Instant"
        />

        <PaymentMethodCard
          icon="phone-portrait-outline"
          title="Mobile Wallet"
          subtitle="Use your wallet app for quick payment"
          tag="Fast"
        />

        <PaymentMethodCard
          icon="cash-outline"
          title="Cash Payment"
          subtitle="Pay at a recharge point or service outlet"
          tag="In Person"
        />

        <View style={styles.demoNote}>
          <Text style={styles.demoNoteText}>
            Payment methods are shown here for the project UI demo.
          </Text>
        </View>
      </View>

      <View style={[styles.sectionCard, styles.sectionCardSpacing]}>
        <Text style={styles.sectionTitle}>Recent Payments</Text>

        {paymentHistory.map((item, index) => (
          <View
            key={item.id}
            style={[
              styles.paymentRow,
              index === paymentHistory.length - 1 && styles.paymentRowLast,
            ]}
          >
            <View>
              <Text style={styles.paymentDate}>{item.date}</Text>
              <Text style={styles.paymentMethod}>{item.method}</Text>
            </View>

            <Text style={styles.paymentAmount}>{item.amount} EGP</Text>
          </View>
        ))}
      </View>
    </ScrollView>
  );
}

function PaymentMethodCard({ icon, title, subtitle, tag }) {
  return (
    <TouchableOpacity style={styles.paymentMethodCard} activeOpacity={0.85}>
      <View style={styles.paymentMethodLeft}>
        <View style={styles.paymentMethodIconWrap}>
          <Ionicons name={icon} size={22} color="#0f766e" />
        </View>

        <View style={styles.paymentMethodTextWrap}>
          <Text style={styles.paymentMethodTitle}>{title}</Text>
          <Text style={styles.paymentMethodSubtitle}>{subtitle}</Text>
        </View>
      </View>

      <View style={styles.paymentMethodTag}>
        <Text style={styles.paymentMethodTagText}>{tag}</Text>
      </View>
    </TouchableOpacity>
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
    maxWidth: '80%',
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
    marginBottom: 20,
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

  billingPreviewCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    marginBottom: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  billingPreviewHeader: {
    marginBottom: 14,
  },
  billingPreviewTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
  },
  billingPreviewSubtitle: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 4,
  },
  billingPreviewRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  billingPreviewItem: {
    width: '48%',
    backgroundColor: '#f8fafc',
    borderRadius: 18,
    padding: 14,
  },
  billingPreviewItemLabel: {
    fontSize: 12,
    color: '#64748b',
    marginBottom: 8,
  },
  billingPreviewItemValue: {
    fontSize: 20,
    fontWeight: '800',
    color: '#0f172a',
  },
  billingPreviewItemDate: {
    fontSize: 12,
    color: '#94a3b8',
    marginTop: 6,
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

  pageTopRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 20,
  },
  backButton: {
    width: 42,
    height: 42,
    borderRadius: 21,
    backgroundColor: '#ffffff',
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    borderColor: '#e2e8f0',
  },
  pageTitle: {
    fontSize: 24,
    fontWeight: '800',
    color: '#0f172a',
  },
  topSpacer: {
    width: 42,
  },

  sectionCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  sectionCardSpacing: {
    marginTop: 16,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 8,
  },

  settingsRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  settingsRowLast: {
    borderBottomWidth: 0,
  },
  settingsRowLeft: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    marginRight: 12,
  },
  settingsIconWrap: {
    width: 42,
    height: 42,
    borderRadius: 21,
    backgroundColor: '#ecfdf5',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  settingsTextWrap: {
    flex: 1,
  },
  settingsRowTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: '#0f172a',
    marginBottom: 4,
  },
  settingsRowSubtitle: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 18,
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

  billingSummaryCard: {
    backgroundColor: '#0f766e',
    borderRadius: 28,
    padding: 22,
    marginBottom: 16,
    shadowColor: '#0f766e',
    shadowOpacity: 0.18,
    shadowRadius: 20,
    shadowOffset: { width: 0, height: 10 },
    elevation: 6,
  },
  billingEyebrow: {
    color: '#ccfbf1',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 0.8,
    marginBottom: 10,
  },
  billingMainValue: {
    color: '#ffffff',
    fontSize: 42,
    fontWeight: '900',
  },
  billingSummaryText: {
    color: '#d1fae5',
    fontSize: 14,
    lineHeight: 20,
    marginTop: 8,
    marginBottom: 18,
  },
  billingStatsRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  billingStatItem: {
    width: '48%',
    backgroundColor: 'rgba(255,255,255,0.14)',
    borderRadius: 18,
    padding: 14,
  },
  billingStatLabel: {
    color: '#ccfbf1',
    fontSize: 12,
    marginBottom: 6,
  },
  billingStatValue: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '800',
  },
  billingStatSubtext: {
    color: '#ccfbf1',
    fontSize: 12,
    marginTop: 6,
  },
  billingSectionHint: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 19,
    marginBottom: 12,
  },
  paymentMethodCard: {
    backgroundColor: '#f8fafc',
    borderRadius: 18,
    padding: 14,
    marginTop: 12,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  paymentMethodLeft: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    marginRight: 12,
  },
  paymentMethodIconWrap: {
    width: 46,
    height: 46,
    borderRadius: 23,
    backgroundColor: '#ecfdf5',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  paymentMethodTextWrap: {
    flex: 1,
  },
  paymentMethodTitle: {
    fontSize: 16,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 4,
  },
  paymentMethodSubtitle: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 18,
  },
  paymentMethodTag: {
    backgroundColor: '#dcfce7',
    borderRadius: 999,
    paddingHorizontal: 10,
    paddingVertical: 6,
  },
  paymentMethodTagText: {
    fontSize: 11,
    fontWeight: '800',
    color: '#166534',
  },
  demoNote: {
    marginTop: 14,
    backgroundColor: '#ecfdf5',
    borderRadius: 16,
    padding: 12,
  },
  demoNoteText: {
    fontSize: 12,
    lineHeight: 18,
    color: '#0f766e',
    fontWeight: '600',
  },
  paymentRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  paymentRowLast: {
    borderBottomWidth: 0,
  },
  paymentDate: {
    fontSize: 15,
    fontWeight: '700',
    color: '#0f172a',
  },
  paymentMethod: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 4,
  },
  paymentAmount: {
    fontSize: 16,
    fontWeight: '800',
    color: '#0f766e',
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
  
heroMetaText: {
  color: '#ccfbf1',
  fontSize: 13,
  marginTop: 10,
  fontWeight: '600',
},
});