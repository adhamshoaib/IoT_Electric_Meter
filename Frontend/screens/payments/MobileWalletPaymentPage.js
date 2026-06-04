import React, { useState } from 'react';
import {
  SafeAreaView,
  ScrollView,
  View,
  Text,
  TextInput,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import styles from '../../styles';

export default function MobileWalletPaymentPage({ onBack, onPaymentSuccess }) {
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