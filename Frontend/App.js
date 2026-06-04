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
import DashboardScreen from './screens/DashboardScreen';
import PaymentMethodCard from './components/PaymentMethodCard';
import BillingScreen from './screens/BillingScreen';
import SettingsScreen from './screens/SettingsScreen';
import ProfileScreen from "./screens/ProfileScreen";

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

  const remainingPayments = paymentHistory.filter(
    (payment) => payment.id !== paymentId
  );

  await remove(
    ref(database, `users/${appUser.uid}/billing/payments/${paymentId}`)
  );

  if (remainingPayments.length === 0) {
    await update(ref(database, `users/${appUser.uid}/billing`), {
      lastTopUpAmount: 0,
      lastTopUpDate: 'No top-up yet',
      lastTopUp: {
        amount: 0,
        date: 'No top-up yet',
      },
    });

    return;
  }

  const newestPayment = remainingPayments[0];

  await update(ref(database, `users/${appUser.uid}/billing`), {
    lastTopUpAmount: Number(newestPayment.amount ?? 0),
    lastTopUpDate: newestPayment.date ?? 'No top-up yet',
    lastTopUp: {
      amount: Number(newestPayment.amount ?? 0),
      date: newestPayment.date ?? 'No top-up yet',
    },
  });
};

const handleClearPayments = async () => {
  if (!appUser?.uid) return;

  await remove(ref(database, `users/${appUser.uid}/billing/payments`));

  await update(ref(database, `users/${appUser.uid}/billing`), {
    lastTopUpAmount: 0,
    lastTopUpDate: 'No top-up yet',
    lastTopUp: {
      amount: 0,
      date: 'No top-up yet',
    },
  });
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

const latestPayment = paymentsList[0];

setPaymentHistory(paymentsList);

setLiveDashboardData((prev) => ({
  ...prev,
  currentBalance: billingData.currentBalance ?? 0,

  lastTopUpAmount: latestPayment ? Number(latestPayment.amount ?? 0) : 0,
  lastTopUpDate: latestPayment ? latestPayment.date : 'No top-up yet',

  lastTopUp: latestPayment
    ? {
        amount: Number(latestPayment.amount ?? 0),
        date: latestPayment.date,
      }
    : {
        amount: 0,
        date: 'No top-up yet',
      },
}));
  });
  return () => unsubscribe;
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










