import React, { useEffect, useState } from 'react';
import {
  SafeAreaView,
  ScrollView,
  View,
  Text,
  TextInput,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import AsyncStorage from '@react-native-async-storage/async-storage';
import styles from '../../styles';

const SAVED_CARD_KEY = 'smart_meter_saved_card';
export default function BankCardPaymentPage({ onBack, onPaymentSuccess }) {
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