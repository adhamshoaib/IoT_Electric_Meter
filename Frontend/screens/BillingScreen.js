import React, { useState } from 'react';
import {
  View,
  Text,
  ScrollView,
  TouchableOpacity,
  TextInput,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import styles from '../styles';
import { calculateEgyptBill } from '../services/calculateEgyptBill';

import PaymentMethodCard from '../components/PaymentMethodCard';
import BankCardPaymentPage from './payments/BankCardPaymentPage';
import MobileWalletPaymentPage from './payments/MobileWalletPaymentPage';
import FawryPaymentPage from './payments/FawryPaymentPage';

export default function BillingScreen({ data, onBack, onTopUpSuccess, payments=[],
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
    require('../assets/payment-logos/visa.png'),
    require('../assets/payment-logos/mc.png'),
    require('../assets/payment-logos/Meeza.svg.png'),
  ]}
  onPress={() => setBillingPage('card')}
/>

<PaymentMethodCard
  icon="phone-portrait-outline"
  title="Mobile Wallet"
  subtitle="Use your wallet app for quick payment"
  logos={[
    require('../assets/payment-logos/vodafone.png'),
    require('../assets/payment-logos/e&.png'),
  ]}
  onPress={() => setBillingPage('wallet')}
/>

<PaymentMethodCard
  icon="receipt-outline"
  title="Fawry"
  subtitle="Pay using a Fawry reference code"
  logos={[
    require('../assets/payment-logos/fawry.png'),
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