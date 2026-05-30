import {
  signInWithEmailAndPassword,
  createUserWithEmailAndPassword,
   signOut,
    deleteUser,
} from "firebase/auth";
import { calculateEgyptBill } from './services/calculateEgyptBill';
import styles from './styles';
import AsyncStorage from '@react-native-async-storage/async-storage';

import { auth } from "./services/firebase";
import React, { useEffect, useState } from 'react';
import {
  ref,
  onValue,
  set as firebaseSet,
  get,
  update,
  push,
  remove,
  runTransaction,
} from 'firebase/database';
import { database } from './services/firebase';
import {
  SafeAreaView,
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  ScrollView,
  KeyboardAvoidingView,
Platform,
Image,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { fakeDashboardData } from './services/fakedata';
import StatisticsScreen from './screens/StatisticsScreen';
const parseFirebaseTimestamp = (timestamp) => {
  if (!timestamp) return 0;

  const asNumber = Number(timestamp);

  if (!Number.isNaN(asNumber)) {
    return asNumber < 10000000000 ? asNumber * 1000 : asNumber;
  }

  const parsedDate = Date.parse(timestamp);
  return Number.isNaN(parsedDate) ? 0 : parsedDate;
};

export default function App() {
  const [fullName, setFullName] = useState('');
const [meterId, setMeterId] = useState('');
const [appUser, setAppUser] = useState(null);
  const [isSignUp, setIsSignUp] = useState(false);
  const [activeScreen, setActiveScreen] = useState('dashboard');
  const [email, setEmail] = useState('');
  const [confirmPassword, setConfirmPassword] = useState("");
  const [authError, setAuthError] = useState("");
  const [authLoading, setAuthLoading] = useState(false);
  const [password, setPassword] = useState("");
const [isLoggedIn, setIsLoggedIn] = useState(false);
const [paymentHistory, setPaymentHistory] = useState([]);
const [liveDashboardData, setLiveDashboardData] = useState({
  ...fakeDashboardData,
  monthlyConsumption: 0,
  todayConsumption: 0,
  currentBalance: 0,
  lastTopUpAmount: 0,
  lastTopUpDate: 'No top-up yet',
  lastTopUp: {
    amount: 0,
    date: 'No top-up yet',
  },
  lastSync: 'Waiting for reading',
});
const handleTopUpSuccess = async (amount, method = 'Payment') => {
  try {
    const topUpAmount = Number(amount);

    if (!topUpAmount || topUpAmount <= 0 || !appUser?.uid) {
      console.log('Top-up stopped:', {
        topUpAmount,
        uid: appUser?.uid,
      });
      return;
    }

    const today = new Date();

    const formattedDate = `${today.getDate()} ${today.toLocaleString('en-US', {
      month: 'short',
    })} ${today.getFullYear()}`;

    const formattedTime = today.toLocaleTimeString('en-US', {
      hour: '2-digit',
      minute: '2-digit',
    });

    const paymentRef = push(
      ref(database, `users/${appUser.uid}/billing/payments`)
    );

    const newPayment = {
      amount: topUpAmount,
      method,
      date: formattedDate,
      time: formattedTime,
      status: 'Successful',
      createdAt: Date.now(),
    };

    await firebaseSet(paymentRef, newPayment);

    await runTransaction(
      ref(database, `users/${appUser.uid}/billing/currentBalance`),
      (currentBalance) => {
        const startingBalance = Number(
          currentBalance ??  0
        );

        return startingBalance + topUpAmount;
      }
    );

    await update(ref(database, `users/${appUser.uid}/billing`), {
      lastTopUpAmount: topUpAmount,
      lastTopUpDate: formattedDate,
      lastTopUp: {
        amount: topUpAmount,
        date: formattedDate,
      },
    });

    console.log('Top-up saved successfully');
  } catch (error) {
    console.log('Top-up error:', error);
  }
};
const handleSetBalance = async (newBalance) => {
  const balanceValue = Number(newBalance);

  if (Number.isNaN(balanceValue) || balanceValue < 0) {
    return false;
  }

  setLiveDashboardData((prev) => ({
    ...prev,
    currentBalance: balanceValue,
  }));

  if (appUser?.uid) {
    await update(ref(database, `users/${appUser.uid}/billing`), {
      currentBalance: balanceValue,
    });
  }

  return true;
};
const handleDeletePayment = async (paymentId) => {
  if (!appUser?.uid) return;

  await remove(ref(database, `users/${appUser.uid}/billing/payments/${paymentId}`));
};

const handleClearPayments = async () => {
  if (!appUser?.uid) return;

  await remove(ref(database, `users/${appUser.uid}/billing/payments`));
};
useEffect(() => {
   if (!isLoggedIn) return;
  const meterRef = ref(database, 'current_reading');

  const unsubscribe = onValue(meterRef, (snapshot) => {
    const firebaseData = snapshot.val();

    if (firebaseData) {
    const readingTime = parseFirebaseTimestamp(firebaseData.ts);

  setLiveDashboardData((prev) => ({
    ...prev,
    monthlyConsumption: firebaseData.energy_kwh ?? prev.monthlyConsumption,
    lastReadingAt: readingTime,
    lastSync: readingTime
      ? new Date(readingTime).toLocaleString('en-US')
      : 'No timestamp',
      }));
    }
  });

  return () => unsubscribe();
}, [isLoggedIn]);
useEffect(() => {
  if (!isLoggedIn || !appUser?.uid) return;

  const billingRef = ref(database, `users/${appUser.uid}/billing`);

  const unsubscribe = onValue(billingRef, (snapshot) => {
    const billingData = snapshot.val();

  if (!billingData) {
  setLiveDashboardData((prev) => ({
    ...prev,
    currentBalance: 0,
    lastTopUpAmount: 0,
    lastTopUpDate: 'No top-up yet',
    lastTopUp: {
      amount: 0,
      date: 'No top-up yet',
    },
  }));

  setPaymentHistory([]);
  return;
}

    setLiveDashboardData((prev) => ({
      ...prev,
      currentBalance: billingData.currentBalance ?? 0,

      lastTopUpAmount: billingData.lastTopUpAmount ?? 0,
      lastTopUpDate: billingData.lastTopUpDate ?? 'No top-up yet',

      lastTopUp: billingData.lastTopUp ?? {
        amount: 0,
        date: 'No top-up yet',
      },
    }));

    const paymentsObject = billingData.payments ?? {};

    const paymentsList = Object.entries(paymentsObject)
      .map(([id, payment]) => ({
        id,
        ...payment,
      }))
      .sort((a, b) => Number(b.createdAt ?? 0) - Number(a.createdAt ?? 0));

    setPaymentHistory(paymentsList);
  });

  return unsubscribe;
}, [isLoggedIn, appUser?.uid]);
const loadUserProfile = async (firebaseUser) => {
  const userRef = ref(database, `users/${firebaseUser.uid}`);
  const snapshot = await get(userRef);

  if (snapshot.exists()) {
    setAppUser(snapshot.val());
  } else {
    const fallbackUser = {
      uid: firebaseUser.uid,
      name: firebaseUser.email?.split('@')[0] ?? 'User',
      email: firebaseUser.email,
      meterId: 'Not assigned',
      accountNumber: firebaseUser.uid.slice(0, 8).toUpperCase(),
      address: 'Not added yet',
    };

    setAppUser(fallbackUser);
  }
};
const handleAuth = async () => {
  setAuthError('');

  const cleanEmail = email.trim();
  const cleanMeterNumber = meterId.trim();
const fullMeterId = `MTR${cleanMeterNumber.padStart(3, '0')}`;

  if (!cleanEmail || !password) {
    setAuthError('Please enter your email and password.');
    return;
  }

  if (isSignUp && !fullName.trim()) {
    setAuthError('Please enter your full name.');
    return;
  }

  if (isSignUp && !meterId.trim()) {
    setAuthError('Please enter your meter ID.');
    return;
  }

  if (isSignUp && password !== confirmPassword) {
    setAuthError('Passwords do not match.');
    return;
  }

  if (isSignUp && password.length < 6) {
    setAuthError('Password should be at least 6 characters.');
    return;
  }

  try {
    setAuthLoading(true);

    if (isSignUp) {
      const userCredential = await createUserWithEmailAndPassword(
        auth,
        cleanEmail,
        password
      );

      const firebaseUser = userCredential.user;
       const meterClaimRef = ref(database, `meterOwners/${fullMeterId}`);
         const claimResult = await runTransaction(meterClaimRef, (currentOwner) => {
    if (currentOwner === null) {
      return firebaseUser.uid;
    }

    return;
  });

  if (!claimResult.committed) {
    await deleteUser(firebaseUser).catch(() => {});

    setAuthError(
      'This meter number is already linked to another account.'
    );

    return;
  }

      const newProfile = {
        uid: firebaseUser.uid,
        name: fullName.trim(),
        email: cleanEmail,
        meterId: fullMeterId,
        accountNumber: firebaseUser.uid.slice(0, 8).toUpperCase(),
        address: 'Not added yet',
      };

      await firebaseSet(ref(database, `users/${firebaseUser.uid}`), newProfile);

      setAppUser(newProfile);
    } else {
      const userCredential = await signInWithEmailAndPassword(
        auth,
        cleanEmail,
        password
      );

      await loadUserProfile(userCredential.user);
    }

    setIsLoggedIn(true);
    setActiveScreen('dashboard');
  } catch (error) {
    console.log(error.code);

    if (error.code === 'auth/email-already-in-use') {
      setAuthError('This email is already registered. Try logging in.');
    } else if (error.code === 'auth/weak-password') {
      setAuthError('Password should be at least 6 characters.');
    } else if (
      error.code === 'auth/invalid-credential' ||
      error.code === 'auth/wrong-password' ||
      error.code === 'auth/user-not-found'
    ) {
      setAuthError('Incorrect email or password.');
    } else if (error.code === 'auth/invalid-email') {
      setAuthError('Please enter a valid email address.');
    } else {
      setAuthError('Something went wrong. Please try again.');
    }
  } finally {
    setAuthLoading(false);
  }
};

if (!isLoggedIn) {
  return (
    <SafeAreaView style={styles.container}>
      <KeyboardAvoidingView
        style={styles.loginKeyboardView}
        behavior={Platform.OS === 'ios' ? 'padding' : undefined}
      >
        <ScrollView
          contentContainerStyle={styles.loginScrollContent}
          showsVerticalScrollIndicator={false}
          keyboardShouldPersistTaps="handled"
        >
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
              <Text style={styles.authTitle}>
                {isSignUp ? 'Create Account' : 'Welcome Back'}
              </Text>

              <Text style={styles.authSubtitle}>
                {isSignUp
                  ? 'Create an account to access your smart meter dashboard.'
                  : 'Login to continue to your smart meter dashboard.'}
              </Text>

              {isSignUp && (
                <>
                  <Text style={styles.inputLabel}>Full Name</Text>
                  <TextInput
                    value={fullName}
                    onChangeText={setFullName}
                    placeholder="Enter your full name"
                    placeholderTextColor="#94a3b8"
                    style={styles.input}
                  />

                  <Text style={styles.inputLabel}>Meter Number</Text>

                  <View style={styles.meterInputRow}>
                    <View style={styles.meterPrefixBox}>
                      <Text style={styles.meterPrefixText}>MTR</Text>
                    </View>

                    <TextInput
                      value={meterId}
                      onChangeText={(text) => {
                        const numbersOnly = text.replace(/[^0-9]/g, '');
                        setMeterId(numbersOnly);
                      }}
                      placeholder="001"
                      keyboardType="number-pad"
                      maxLength={3}
                      placeholderTextColor="#94a3b8"
                      style={styles.meterNumberInput}
                    />
                  </View>

                  {meterId ? (
                    <Text style={styles.meterPreviewText}>
                      Full Meter ID: MTR{meterId.padStart(3, '0')}
                    </Text>
                  ) : null}
                </>
              )}

              <Text style={styles.inputLabel}>Email</Text>
              <TextInput
                value={email}
                onChangeText={setEmail}
                placeholder="Enter your email"
                autoCapitalize="none"
                keyboardType="email-address"
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

              {isSignUp && (
                <>
                  <Text style={styles.inputLabel}>Confirm Password</Text>
                  <TextInput
                    value={confirmPassword}
                    onChangeText={setConfirmPassword}
                    placeholder="Re-enter your password"
                    secureTextEntry
                    placeholderTextColor="#94a3b8"
                    style={styles.input}
                  />
                </>
              )}

              {authError ? (
                <Text style={styles.authError}>{authError}</Text>
              ) : null}

              <TouchableOpacity
                style={[styles.loginButton, authLoading && styles.disabledButton]}
                onPress={handleAuth}
                disabled={authLoading}
              >
                <Text style={styles.loginButtonText}>
                  {authLoading ? 'Please wait...' : isSignUp ? 'Sign Up' : 'Login'}
                </Text>
              </TouchableOpacity>

              <TouchableOpacity
                onPress={() => {
                  setIsSignUp(!isSignUp);
                  setAuthError('');
                  setPassword('');
                  setConfirmPassword('');
                }}
              >
                <Text style={styles.signUpText}>
                  {isSignUp
                    ? 'Already have an account? Login'
                    : "Don't have an account? Sign Up"}
                </Text>
              </TouchableOpacity>
            </View>
          </View>
        </ScrollView>
      </KeyboardAvoidingView>
    </SafeAreaView>
  );
}

  return (
    <SafeAreaView style={styles.container}>
      {activeScreen === 'dashboard' && (
        <DashboardScreen
  user={appUser}
  data={liveDashboardData}
  onOpenSettings={() => setActiveScreen('settings')}
/>
      )}

      {activeScreen === 'settings' && (
        <SettingsScreen
          onBack={() => setActiveScreen('dashboard')}
          onOpenProfile={() => setActiveScreen('profile')}
          onOpenBilling={() => setActiveScreen('billing')}
        onLogout={async () => {
  await signOut(auth);
    setAppUser(null);
  setIsLoggedIn(false);
  setActiveScreen('dashboard');
}}
        />
      )}

      {activeScreen === 'profile' && (
       <ProfileScreen
  user={appUser}
  onBack={() => setActiveScreen('settings')}
/>
      )}

      {activeScreen === 'statistics' && (
        <StatisticsScreen onBack={() => setActiveScreen('dashboard')} />
      )}

      {activeScreen === 'billing' && (
 <BillingScreen
  data={liveDashboardData}
  onBack={() => setActiveScreen('dashboard')}
  onTopUpSuccess={handleTopUpSuccess}
  payments={paymentHistory}
  onDeletePayment={handleDeletePayment}
  onClearPayments={handleClearPayments}
  onSetBalance={handleSetBalance}
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
  const [now, setNow] = useState(Date.now());

useEffect(() => {
  const timer = setInterval(() => {
    setNow(Date.now());
  }, 10000);

  return () => clearInterval(timer);
}, []);
const consumptionValue = Number(data?.monthlyConsumption ?? 0);
const estimatedBill = calculateEgyptBill(consumptionValue);
const currentBalance = Number(data?.currentBalance ?? 0);
const availableBalance = Math.max(currentBalance - estimatedBill.totalCost, 0);
const lastReadingAt = Number(data?.lastReadingAt ?? 0);
const readingAgeSeconds = lastReadingAt ? (now - lastReadingAt) / 1000 : Infinity;

const isMeterOnline = readingAgeSeconds <= 60;
const lowBalanceThreshold = 50;
const isLowBalance = availableBalance > 0 && availableBalance < lowBalanceThreshold;
const isBalanceEmpty = availableBalance <= 0;
const lastTopUpAmount = Number(data?.lastTopUpAmount ?? 0);
const lastTopUpDate = data?.lastTopUpDate ?? 'No top-up yet';
const isMeterOff = isBalanceEmpty || !isMeterOnline;

const meterStatusTitle = isMeterOff ? 'Meter Off' : 'Meter On';

const meterStatusSubtitle = isBalanceEmpty
  ? 'Recharge required'
  : !isMeterOnline
    ? 'No recent meter readings'
    : 'Receiving readings normally';

const meterStatusColor = isMeterOff ? '#ef4444' : '#22c55e';
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
          <Text style={styles.balanceSectionLabel}>Available Balance</Text>
          <Text style={styles.balanceSectionValue}>
            {availableBalance.toFixed(2)} EGP
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
             {estimatedBill.totalCost.toFixed(2)} EGP
            </Text>
          </View>

          <View style={styles.billingPreviewItem}>
            <Text style={styles.billingPreviewItemLabel}>Last Top-Up</Text>
            <Text style={styles.billingPreviewItemValue}>
            {lastTopUpAmount.toFixed(2)} EGP
            </Text>
            <Text style={styles.billingPreviewItemDate}>{lastTopUpDate}</Text>
          </View>
        </View>
      </View>

      <View style={styles.divider} />

      <View style={styles.statusSection}>
        <View style={styles.statusIconArea}>
          <View
  style={[
    styles.signalDotOuter,
    { backgroundColor: `${meterStatusColor}20` },
  ]}
>
  <View
    style={[
      styles.signalDotInner,
      { backgroundColor: meterStatusColor },
    ]}
  />
</View>

<View style={styles.barsWrap}>
  <View style={[styles.bar, styles.barOne, { backgroundColor: meterStatusColor }]} />
  <View style={[styles.bar, styles.barTwo, { backgroundColor: meterStatusColor }]} />
  <View style={[styles.bar, styles.barThree, { backgroundColor: meterStatusColor }]} />
  <View style={[styles.bar, styles.barFour, { backgroundColor: meterStatusColor }]} />
</View>
        </View>

        <View style={styles.statusTextWrap}>
          <Text style={styles.statusEyebrow}>METER STATUS</Text>
        <Text
    style={[
      styles.meterStatusTitle,
      { color: meterStatusColor },
    ]}
    
  >
    {meterStatusTitle}
  </Text>

  <Text style={styles.meterStatusSubtitle}>
    {meterStatusSubtitle}
  </Text>
        </View>

        <View style={styles.syncPill}>
          <Text style={styles.syncPillText}>
            {data.lastSync ?? '2 min ago'}
          </Text>
        </View>
      </View>
{(isLowBalance || isBalanceEmpty) && (
  <View
    style={[
      styles.balanceAlertCard,
      isBalanceEmpty && styles.balanceAlertCardDanger,
    ]}
  >
    <View
      style={[
        styles.balanceAlertIconWrap,
        isBalanceEmpty && styles.balanceAlertIconWrapDanger,
      ]}
    >
      <Ionicons
        name={isBalanceEmpty ? 'close-circle-outline' : 'warning-outline'}
        size={24}
        color="#ffffff"
      />
    </View>

    <View style={styles.balanceAlertTextWrap}>
      <Text
        style={[
          styles.balanceAlertTitle,
          isBalanceEmpty && styles.balanceAlertTitleDanger,
        ]}
      >
        {isBalanceEmpty ? 'Balance Empty' : 'Low Balance Warning'}
      </Text>

      <Text style={styles.balanceAlertSubtitle}>
        {isBalanceEmpty
          ? 'Your prepaid balance is empty. Please recharge to restore service.'
          : 'Your available balance is low. Please recharge soon.'}
      </Text>
    </View>
  </View>
)}
    
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
const SAVED_CARD_KEY = 'smart_meter_saved_card';
function BankCardPaymentPage({ onBack, onPaymentSuccess }) {
  const [amount, setAmount] = useState('');
  const [cardHolder, setCardHolder] = useState('');
  const [cardNumber, setCardNumber] = useState('');
  const [expiryDate, setExpiryDate] = useState('');
  const [cvv, setCvv] = useState('');
  const [status, setStatus] = useState('idle');
  const [message, setMessage] = useState('');

  const [savedCard, setSavedCard] = useState(null);
  const [useSavedCard, setUseSavedCard] = useState(false);
  const [rememberCard, setRememberCard] = useState(true);

  useEffect(() => {
    loadSavedCard();
  }, []);

  const loadSavedCard = async () => {
    try {
      const storedCard = await AsyncStorage.getItem(SAVED_CARD_KEY);

      if (storedCard) {
        const parsedCard = JSON.parse(storedCard);
        setSavedCard(parsedCard);
        setUseSavedCard(true);
      }
    } catch (error) {
      console.log('Error loading saved card:', error);
    }
  };

  const removeSavedCard = async () => {
    try {
      await AsyncStorage.removeItem(SAVED_CARD_KEY);
      setSavedCard(null);
      setUseSavedCard(false);
      setMessage('Saved card removed.');
    } catch (error) {
      setMessage('Could not remove saved card.');
    }
  };

  const getCardBrand = (number) => {
    const cleanNumber = number.replace(/\s/g, '');

    if (cleanNumber.startsWith('4')) return 'Visa';
    if (cleanNumber.startsWith('5')) return 'Mastercard';

    return 'Bank Card';
  };

  const saveCardLocally = async () => {
    const cleanNumber = cardNumber.replace(/\s/g, '');
    const last4 = cleanNumber.slice(-4);

    const cardToSave = {
      holder: cardHolder.trim(),
      last4,
      expiry: expiryDate.trim(),
      brand: getCardBrand(cleanNumber),
    };

    await AsyncStorage.setItem(SAVED_CARD_KEY, JSON.stringify(cardToSave));
    setSavedCard(cardToSave);
    setUseSavedCard(true);
  };

  const handlePay = async () => {
    const topUpAmount = Number(amount);

    if (!topUpAmount || topUpAmount <= 0) {
      setMessage('Please enter a valid top-up amount.');
      return;
    }

   if (useSavedCard && savedCard) {
  setStatus('processing');
  setMessage('Processing payment...');

  setTimeout(() => {
    onPaymentSuccess?.(topUpAmount, 'Bank Card');

    setStatus('success');
    setMessage(`Payment successful. ${topUpAmount.toFixed(2)} EGP added to your balance.`);
  }, 1800);

  return;
}
    if (!cardHolder.trim()) {
      setMessage('Please enter the card holder name.');
      return;
    }

    if (cardNumber.replace(/\s/g, '').length < 16) {
      setMessage('Please enter a valid card number.');
      return;
    }

    if (!expiryDate.trim()) {
      setMessage('Please enter the expiry date.');
      return;
    }

    if (cvv.length < 3) {
      setMessage('Please enter a valid CVV.');
      return;
    }

    setStatus('processing');
    setMessage('Processing payment...');

    setTimeout(async () => {
      if (rememberCard) {
        await saveCardLocally();
      }
  onPaymentSuccess?.(topUpAmount, 'Bank Card');
      setStatus('success');
      setMessage(`Payment successful. ${topUpAmount.toFixed(2)} EGP added to your balance.`);
    }, 1800);
  };

  const formatCardNumber = (text) => {
    const numbersOnly = text.replace(/[^0-9]/g, '');
    return numbersOnly.replace(/(.{4})/g, '$1 ').trim();
  };

  const formatExpiryDate = (text) => {
    const numbersOnly = text.replace(/[^0-9]/g, '');

    if (numbersOnly.length <= 2) {
      return numbersOnly;
    }

    return `${numbersOnly.slice(0, 2)}/${numbersOnly.slice(2, 4)}`;
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.paymentPageContent}>
        <TouchableOpacity style={styles.paymentBackButton} onPress={onBack}>
          <Text style={styles.paymentBackText}>← Back</Text>
        </TouchableOpacity>

        <View style={styles.cardVisual}>
          <Text style={styles.cardVisualLabel}>
            {useSavedCard && savedCard ? savedCard.brand : 'Smart Meter Card'}
          </Text>

          <Text style={styles.cardVisualNumber}>
            {useSavedCard && savedCard
              ? `•••• •••• •••• ${savedCard.last4}`
              : cardNumber || '4242 4242 4242 4242'}
          </Text>

          <View style={styles.cardVisualBottom}>
            <View>
              <Text style={styles.cardVisualSmallLabel}>Card Holder</Text>
              <Text style={styles.cardVisualValue}>
                {useSavedCard && savedCard
                  ? savedCard.holder
                  : cardHolder || 'YOUR NAME'}
              </Text>
            </View>

            <View>
              <Text style={styles.cardVisualSmallLabel}>Expires</Text>
              <Text style={styles.cardVisualValue}>
                {useSavedCard && savedCard
                  ? savedCard.expiry
                  : expiryDate || '12/28'}
              </Text>
            </View>
          </View>
        </View>

        <View style={styles.paymentFormCard}>
          <Text style={styles.paymentPageTitle}>Bank Card Payment</Text>
          <Text style={styles.paymentPageSubtitle}>
            Enter your card details to complete the top-up.
          </Text>

          {savedCard && useSavedCard && (
            <View style={styles.savedCardBox}>
              <View>
                <Text style={styles.savedCardTitle}>
                  {savedCard.brand} •••• {savedCard.last4}
                </Text>
                <Text style={styles.savedCardSubtitle}>
                  {savedCard.holder} · Expires {savedCard.expiry}
                </Text>
              </View>

              <TouchableOpacity onPress={() => setUseSavedCard(false)}>
                <Text style={styles.changeCardText}>Change</Text>
              </TouchableOpacity>
            </View>
          )}

          <Text style={styles.inputLabel}>Top-up Amount</Text>
          <TextInput
            style={styles.input}
            value={amount}
            onChangeText={setAmount}
            placeholder="Enter amount in EGP"
            keyboardType="numeric"
            placeholderTextColor="#94a3b8"
          />

          {(!savedCard || !useSavedCard) && (
            <>
              <Text style={styles.inputLabel}>Card Holder Name</Text>
              <TextInput
                style={styles.input}
                value={cardHolder}
                onChangeText={setCardHolder}
                placeholder="Mohannad Hany"
                placeholderTextColor="#94a3b8"
              />

              <Text style={styles.inputLabel}>Card Number</Text>
              <TextInput
                style={styles.input}
                value={cardNumber}
                onChangeText={(text) => setCardNumber(formatCardNumber(text))}
                placeholder="4242 4242 4242 4242"
                keyboardType="number-pad"
                maxLength={19}
                placeholderTextColor="#94a3b8"
              />

              <View style={styles.cardInputRow}>
                <View style={{ flex: 1 }}>
                  <Text style={styles.inputLabel}>Expiry</Text>
                  <TextInput
                    style={styles.input}
                    value={expiryDate}
                    onChangeText={(text) => setExpiryDate(formatExpiryDate(text))}
                    placeholder="12/28"
                    keyboardType="number-pad"
                    maxLength={5}
                    placeholderTextColor="#94a3b8"
                  />
                </View>

                <View style={{ flex: 1 }}>
                  <Text style={styles.inputLabel}>CVV</Text>
                  <TextInput
                    style={styles.input}
                    value={cvv}
                    onChangeText={(text) => {
                      const numbersOnly = text.replace(/[^0-9]/g, '');
                      setCvv(numbersOnly);
                    }}
                    placeholder="123"
                    keyboardType="number-pad"
                    maxLength={3}
                    secureTextEntry
                    placeholderTextColor="#94a3b8"
                  />
                </View>
              </View>

              <TouchableOpacity
                style={styles.rememberCardRow}
                onPress={() => setRememberCard(!rememberCard)}
                activeOpacity={0.8}
              >
                <View
                  style={[
                    styles.rememberCheckbox,
                    rememberCard && styles.rememberCheckboxActive,
                  ]}
                >
                  {rememberCard && (
                    <Ionicons name="checkmark" size={14} color="#ffffff" />
                  )}
                </View>

                <Text style={styles.rememberCardText}>
                  Remember this card for future payments
                </Text>
              </TouchableOpacity>
            </>
          )}

          <TouchableOpacity
            style={[
              styles.payNowButton,
              status === 'processing' && { opacity: 0.7 },
            ]}
            onPress={handlePay}
            disabled={status === 'processing'}
          >
            <Text style={styles.payNowButtonText}>
              {status === 'processing' ? 'Processing...' : 'Pay Now'}
            </Text>
          </TouchableOpacity>

          {savedCard && (
            <TouchableOpacity
              style={styles.removeSavedCardButton}
              onPress={removeSavedCard}
            >
              <Text style={styles.removeSavedCardText}>Remove saved card</Text>
            </TouchableOpacity>
          )}

          {!!message && (
            <Text
              style={[
                styles.paymentStatusMessage,
                status === 'success' && styles.paymentStatusSuccess,
              ]}
            >
              {message}
            </Text>
          )}
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}
function MobileWalletPaymentPage({ onBack, onPaymentSuccess }) {
  const [amount, setAmount] = useState('');
  const [walletNumber, setWalletNumber] = useState('');
  const [otp, setOtp] = useState('');
  const [step, setStep] = useState('details');
  const [status, setStatus] = useState('idle');
  const [message, setMessage] = useState('');

  const handleSendOtp = () => {
    const topUpAmount = Number(amount);
    const cleanNumber = walletNumber.replace(/\s/g, '');

    if (!topUpAmount || topUpAmount <= 0) {
      setMessage('Please enter a valid top-up amount.');
      return;
    }

    if (cleanNumber.length !== 11 || !cleanNumber.startsWith('01')) {
      setMessage('Please enter a valid wallet phone number.');
      return;
    }

    setStatus('processing');
    setMessage('Sending verification code...');

    setTimeout(() => {
      setStatus('idle');
      setStep('otp');
      setMessage('Verification code sent to your mobile wallet number.');
    }, 1500);
  };

  const handleConfirmPayment = () => {
    if (otp.length !== 6) {
      setMessage('Please enter the 6-digit verification code.');
      return;
    }

    setStatus('processing');
    setMessage('Confirming wallet payment...');

   setTimeout(() => {
  const topUpAmount = Number(amount);

  onPaymentSuccess?.(topUpAmount, 'Mobile Wallet');

  setStatus('success');
  setMessage(`Payment successful. ${topUpAmount.toFixed(2)} EGP added to your balance.`);
}, 1800);
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.paymentPageContent}>
        <TouchableOpacity style={styles.paymentBackButton} onPress={onBack}>
          <Text style={styles.paymentBackText}>← Back</Text>
        </TouchableOpacity>

        <View style={styles.walletHeroCard}>
          <View style={styles.walletHeroIcon}>
            <Ionicons name="phone-portrait-outline" size={30} color="#0f766e" />
          </View>

          <Text style={styles.walletHeroTitle}>Mobile Wallet</Text>
          <Text style={styles.walletHeroSubtitle}>
            Recharge your meter balance using your mobile wallet number.
          </Text>
        </View>

        <View style={styles.paymentFormCard}>
          <Text style={styles.paymentPageTitle}>Wallet Payment</Text>
          <Text style={styles.paymentPageSubtitle}>
            Enter your wallet number and confirm the payment using the verification code.
          </Text>

          <Text style={styles.inputLabel}>Top-up Amount</Text>
          <TextInput
            style={styles.input}
            value={amount}
            onChangeText={setAmount}
            placeholder="Enter amount in EGP"
            keyboardType="numeric"
            placeholderTextColor="#94a3b8"
            editable={step === 'details'}
          />

          <Text style={styles.inputLabel}>Wallet Phone Number</Text>
          <TextInput
            style={styles.input}
            value={walletNumber}
            onChangeText={(text) => {
              const numbersOnly = text.replace(/[^0-9]/g, '');
              setWalletNumber(numbersOnly);
            }}
            placeholder="01XXXXXXXXX"
            keyboardType="number-pad"
            maxLength={11}
            placeholderTextColor="#94a3b8"
            editable={step === 'details'}
          />

          {step === 'otp' && (
            <>
              <Text style={styles.inputLabel}>Verification Code</Text>
              <TextInput
                style={styles.input}
                value={otp}
                onChangeText={(text) => {
                  const numbersOnly = text.replace(/[^0-9]/g, '');
                  setOtp(numbersOnly);
                }}
                placeholder="Enter 6-digit code"
                keyboardType="number-pad"
                maxLength={6}
                placeholderTextColor="#94a3b8"
              />

             
            </>
          )}

          {step === 'details' ? (
            <TouchableOpacity
              style={[
                styles.payNowButton,
                status === 'processing' && { opacity: 0.7 },
              ]}
              onPress={handleSendOtp}
              disabled={status === 'processing'}
            >
              <Text style={styles.payNowButtonText}>
                {status === 'processing' ? 'Sending...' : 'Send Verification Code'}
              </Text>
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={[
                styles.payNowButton,
                status === 'processing' && { opacity: 0.7 },
              ]}
              onPress={handleConfirmPayment}
              disabled={status === 'processing' || status === 'success'}
            >
              <Text style={styles.payNowButtonText}>
                {status === 'processing' ? 'Confirming...' : 'Confirm Payment'}
              </Text>
            </TouchableOpacity>
          )}

          {!!message && (
            <Text
              style={[
                styles.paymentStatusMessage,
                status === 'success' && styles.paymentStatusSuccess,
              ]}
            >
              {message}
            </Text>
          )}
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}
function FawryPaymentPage({ onBack, onPaymentSuccess }) {
  const [amount, setAmount] = useState('');
  const [phoneNumber, setPhoneNumber] = useState('');
  const [referenceCode, setReferenceCode] = useState('');
  const [expiryTime, setExpiryTime] = useState('');
  const [step, setStep] = useState('details');
  const [status, setStatus] = useState('idle');
  const [message, setMessage] = useState('');
const formatGregorianDate = (date) => {
  const months = [
    'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
    'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
  ];

  let hours = date.getHours();
  const minutes = String(date.getMinutes()).padStart(2, '0');
  const ampm = hours >= 12 ? 'PM' : 'AM';

  hours = hours % 12;
  hours = hours || 12;

  return `${date.getDate()} ${months[date.getMonth()]} ${date.getFullYear()}, ${hours}:${minutes} ${ampm}`;
};
  const generateReferenceCode = () => {
    const topUpAmount = Number(amount);
    const cleanPhone = phoneNumber.replace(/\s/g, '');

    if (!topUpAmount || topUpAmount <= 0) {
      setMessage('Please enter a valid top-up amount.');
      return;
    }

    if (cleanPhone.length !== 11 || !cleanPhone.startsWith('01')) {
      setMessage('Please enter a valid phone number.');
      return;
    }

    const generatedCode = Math.floor(100000000 + Math.random() * 900000000).toString();

    const expiryDate = new Date(Date.now() + 24 * 60 * 60 * 1000);
const expiry = formatGregorianDate(expiryDate);

    setReferenceCode(generatedCode);
    setExpiryTime(expiry);
    setStep('reference');
    setStatus('idle');
    setMessage('');
  };

  const handleConfirmPayment = () => {
    setStatus('processing');
    setMessage('Checking payment status...');

    setTimeout(() => {
  const topUpAmount = Number(amount);

  onPaymentSuccess?.(topUpAmount, 'Fawry');

  setStatus('success');
  setMessage(`Payment confirmed. ${topUpAmount.toFixed(2)} EGP added to your balance.`);
}, 1800);
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.paymentPageContent}>
        <TouchableOpacity style={styles.paymentBackButton} onPress={onBack}>
          <Text style={styles.paymentBackText}>← Back</Text>
        </TouchableOpacity>

        <View style={styles.fawryHeroCard}>
          <View style={styles.fawryHeroTop}>
            <View style={styles.fawryHeroIcon}>
              <Ionicons name="receipt-outline" size={30} color="#1e3a8a" />
            </View>

            <Image
              source={require('./assets/payment-logos/fawry.png')}
              style={styles.fawryHeroLogo}
            />
          </View>

          <Text style={styles.fawryHeroTitle}>Fawry Payment</Text>
          <Text style={styles.fawryHeroSubtitle}>
            Generate a reference code and pay through any Fawry outlet or supported app.
          </Text>
        </View>

        <View style={styles.paymentFormCard}>
          <Text style={styles.paymentPageTitle}>Fawry Reference</Text>
          <Text style={styles.paymentPageSubtitle}>
            Enter the top-up details to generate your payment reference.
          </Text>

          <Text style={styles.inputLabel}>Top-up Amount</Text>
          <TextInput
            style={styles.input}
            value={amount}
            onChangeText={setAmount}
            placeholder="Enter amount in EGP"
            keyboardType="numeric"
            placeholderTextColor="#94a3b8"
            editable={step === 'details'}
          />

          <Text style={styles.inputLabel}>Phone Number</Text>
          <TextInput
            style={styles.input}
            value={phoneNumber}
            onChangeText={(text) => {
              const numbersOnly = text.replace(/[^0-9]/g, '');
              setPhoneNumber(numbersOnly);
            }}
            placeholder="01XXXXXXXXX"
            keyboardType="number-pad"
            maxLength={11}
            placeholderTextColor="#94a3b8"
            editable={step === 'details'}
          />

          {step === 'details' && (
            <TouchableOpacity style={styles.payNowButton} onPress={generateReferenceCode}>
              <Text style={styles.payNowButtonText}>Generate Reference Code</Text>
            </TouchableOpacity>
          )}

          {step === 'reference' && (
            <View style={styles.fawryReferenceBox}>
              <Text style={styles.fawryReferenceLabel}>Payment Reference Code</Text>
              <Text style={styles.fawryReferenceCode}>{referenceCode}</Text>

              <View style={styles.fawryInfoRow}>
                <Text style={styles.fawryInfoLabel}>Amount</Text>
                <Text style={styles.fawryInfoValue}>
                  {Number(amount).toFixed(2)} EGP
                </Text>
              </View>

              <View style={styles.fawryInfoRow}>
                <Text style={styles.fawryInfoLabel}>Phone</Text>
                <Text style={styles.fawryInfoValue}>{phoneNumber}</Text>
              </View>

              <View style={styles.fawryInfoRow}>
                <Text style={styles.fawryInfoLabel}>Expires</Text>
                <Text style={styles.fawryInfoValue}>{expiryTime}</Text>
              </View>

              <View style={styles.fawryStepsBox}>
                <Text style={styles.fawryStepsTitle}>How to pay</Text>
                <Text style={styles.fawryStepText}>1. Open your Fawry app or visit a Fawry outlet.</Text>
                <Text style={styles.fawryStepText}>2. Choose bill payment or Fawry Pay.</Text>
                <Text style={styles.fawryStepText}>3. Enter the reference code shown above.</Text>
                <Text style={styles.fawryStepText}>4. Complete the payment, then confirm below.</Text>
              </View>

              <TouchableOpacity
                style={[
                  styles.payNowButton,
                  status === 'processing' && { opacity: 0.7 },
                ]}
                onPress={handleConfirmPayment}
                disabled={status === 'processing' || status === 'success'}
              >
                <Text style={styles.payNowButtonText}>
                  {status === 'processing' ? 'Checking...' : 'I Have Paid'}
                </Text>
              </TouchableOpacity>
            </View>
          )}

          {!!message && (
            <Text
              style={[
                styles.paymentStatusMessage,
                status === 'success' && styles.paymentStatusSuccess,
              ]}
            >
              {message}
            </Text>
          )}
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}
function BillingScreen({ data, onBack, onTopUpSuccess, payments=[],
  onDeletePayment,
  onClearPayments, onSetBalance,}) {
const [billingPage, setBillingPage] = useState('methods');
const [balanceInput, setBalanceInput] = useState('');
const [balanceMessage, setBalanceMessage] = useState('');
  const consumptionValue = Number(data?.monthlyConsumption ?? 0);
const estimatedBill = calculateEgyptBill(consumptionValue);
const estimatedCost = estimatedBill.totalCost;

const currentBalance   = Number(data?.currentBalance ?? 0);
const storedBalance = Math.max(storedBalance - estimatedCost, 0);
const paymentHistory = payments;
  
  const lastTopUpAmount = data.lastTopUp?.amount ?? 0;
  const lastTopUpDate = data.lastTopUp?.date ?? 'No top-up yet';
  if (billingPage === 'card') {
  return (
    <BankCardPaymentPage
      onBack={() => setBillingPage('methods')}
        onPaymentSuccess={onTopUpSuccess}
    />
  );
}
if (billingPage === 'wallet') {
  return (
    <MobileWalletPaymentPage
      onBack={() => setBillingPage('methods')}
         onPaymentSuccess={onTopUpSuccess}
    />
  );
}
if (billingPage === 'fawry') {
  return (
    <FawryPaymentPage
      onBack={() => setBillingPage('methods')}
      onPaymentSuccess={onTopUpSuccess}
    />
  );
}

const handleBankCardPayment = () => {
  const amount = Number(topUpAmount);

  if (!amount || amount <= 0) {
    setPaymentMessage('Please enter a valid top-up amount.');
    return;
  }

  if (!cardHolder.trim()) {
    setPaymentMessage('Please enter the card holder name.');
    return;
  }

  if (cardNumber.replace(/\s/g, '').length < 16) {
    setPaymentMessage('Please enter a valid demo card number.');
    return;
  }

  if (!expiryDate.trim()) {
    setPaymentMessage('Please enter the expiry date.');
    return;
  }

  if (cvv.length < 3) {
    setPaymentMessage('Please enter a valid CVV.');
    return;
  }

  setPaymentStatus('processing');
  setPaymentMessage('Processing bank card payment...');

  setTimeout(() => {
    setPaymentStatus('success');
    setDemoBalance((prev) => prev + amount);
    setPaymentMessage(`Payment successful. ${amount.toFixed(2)} EGP added to your balance.`);
  }, 1800);
};
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
  <Text style={styles.sectionTitle}>Balance Control</Text>

  <Text style={styles.balanceControlSubtitle}>
    Set the meter balance for testing.
  </Text>

  <TextInput
    style={styles.input}
    value={balanceInput}
    onChangeText={setBalanceInput}
    placeholder="Enter balance in EGP"
    keyboardType="numeric"
    placeholderTextColor="#94a3b8"
  />

  <TouchableOpacity
    style={styles.payNowButton}
    onPress={async () => {
      const success = await onSetBalance?.(balanceInput);

      if (success) {
        setBalanceMessage(`Balance set to ${Number(balanceInput).toFixed(2)} EGP`);
        setBalanceInput('');
      } else {
        setBalanceMessage('Please enter a valid balance.');
      }
    }}
  >
    <Text style={styles.payNowButtonText}>Set Balance</Text>
  </TouchableOpacity>

  {!!balanceMessage && (
    <Text style={styles.paymentStatusMessage}>
      {balanceMessage}
    </Text>
  )}
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
  logos={[
    require('./assets/payment-logos/visa.png'),
    require('./assets/payment-logos/mc.png'),
    require('./assets/payment-logos/Meeza.svg.png'),
  ]}
  onPress={() => setBillingPage('card')}
/>

<PaymentMethodCard
  icon="phone-portrait-outline"
  title="Mobile Wallet"
  subtitle="Use your wallet app for quick payment"
  logos={[
    require('./assets/payment-logos/vodafone.png'),
    require('./assets/payment-logos/e&.png'),
  ]}
  onPress={() => setBillingPage('wallet')}
/>

<PaymentMethodCard
  icon="receipt-outline"
  title="Fawry"
  subtitle="Pay using a Fawry reference code"
  logos={[
    require('./assets/payment-logos/fawry.png'),
  ]}
  logoSize="large"
  onPress={() => setBillingPage('fawry')}
/>
  

        
      </View>
<View style={[styles.sectionCard, styles.sectionCardSpacing]}>
  <View style={styles.recentPaymentsHeader}>
    <View>
      <Text style={styles.sectionTitle}>Recent Payments</Text>
      <Text style={styles.recentPaymentsSubtitle}>
        Your latest successful top-ups
      </Text>
    </View>

    {paymentHistory.length > 0 && (
      <TouchableOpacity onPress={onClearPayments}>
        <Text style={styles.clearPaymentsText}>Clear All</Text>
      </TouchableOpacity>
    )}
  </View>

  {paymentHistory.length === 0 ? (
    <View style={styles.emptyPaymentsBox}>
      <Text style={styles.emptyPaymentsText}>No payments yet</Text>
    </View>
  ) : (
    paymentHistory.map((item, index) => (
      <View
        key={item.id}
        style={[
          styles.paymentRow,
          index === paymentHistory.length - 1 && styles.paymentRowLast,
        ]}
      >
        <View style={{ flex: 1 }}>
          <Text style={styles.paymentDate}>
            {item.date} · {item.time}
          </Text>

          <Text style={styles.paymentMethod}>
            {item.method}
          </Text>

          <Text style={styles.paymentStatusText}>
            {item.status}
          </Text>
        </View>

        <View style={styles.paymentRightSide}>
          <Text style={styles.paymentAmount}>
            +{Number(item.amount).toFixed(2)} EGP
          </Text>

          <TouchableOpacity
            style={styles.deletePaymentButton}
            onPress={() => onDeletePayment(item.id)}
          >
            <Text style={styles.deletePaymentText}>Delete</Text>
          </TouchableOpacity>
        </View>
      </View>
    ))
  )}
</View>
     
       
    </ScrollView>
  );
}

function PaymentMethodCard({ icon, title, subtitle, logos = [], logoSize = 'normal', onPress }) {
  return (
    <TouchableOpacity
      style={styles.paymentMethodCard}
      activeOpacity={0.85}
      onPress={onPress}
    >
      <View style={styles.paymentMethodLeft}>
        <View style={styles.paymentMethodIconWrap}>
          <Ionicons name={icon} size={22} color="#0f766e" />
        </View>

        <View style={styles.paymentMethodTextWrap}>
          <Text style={styles.paymentMethodTitle}>{title}</Text>
          <Text style={styles.paymentMethodSubtitle}>{subtitle}</Text>
        </View>
      </View>

      <View style={styles.paymentLogoInlineRow}>
        {logos.map((logo, index) => (
          <React.Fragment key={index}>
            <Image
  source={logo}
  style={[
    styles.paymentLogoInlineImage,
    title === 'Mobile Wallet' && index === 0 && styles.vodafoneLogoImage,
    title === 'Fawry' && styles.fawryLogoImage,
  ]}
/>

            {index < logos.length - 1 && (
              <Text style={styles.paymentLogoComma}>,</Text>
            )}
          </React.Fragment>
        ))}
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

