import React, { useState } from 'react';
import {
  SafeAreaView,
  ScrollView,
  View,
  Text,
  TextInput,
  TouchableOpacity,
  Image,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import styles from '../../styles';

export default function FawryPaymentPage({ onBack, onPaymentSuccess }) {
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
              source={require('../../assets/payment-logos/fawry.png')}
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