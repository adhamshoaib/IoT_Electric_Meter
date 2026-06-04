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

const SAVED_CARDS_KEY = 'smart_meter_saved_cards';

export default function BankCardPaymentPage({ onBack, onPaymentSuccess }) {
  const [amount, setAmount] = useState('');
  const [cardHolder, setCardHolder] = useState('');
  const [cardNumber, setCardNumber] = useState('');
  const [expiryDate, setExpiryDate] = useState('');
  const [cvv, setCvv] = useState('');
  const [status, setStatus] = useState('idle');
  const [message, setMessage] = useState('');

  const [savedCards, setSavedCards] = useState([]);
  const [selectedCardId, setSelectedCardId] = useState(null);
  const [isAddingNewCard, setIsAddingNewCard] = useState(false);
  const [rememberCard, setRememberCard] = useState(true);

  useEffect(() => {
    loadSavedCards();
  }, []);

  const loadSavedCards = async () => {
    try {
      const storedCards = await AsyncStorage.getItem(SAVED_CARDS_KEY);

      if (storedCards) {
        const parsedCards = JSON.parse(storedCards);
        setSavedCards(parsedCards);

        if (parsedCards.length > 0) {
          setSelectedCardId(parsedCards[0].id);
          setIsAddingNewCard(false);
        }
      } else {
        setIsAddingNewCard(true);
      }
    } catch (error) {
      console.log('Error loading saved cards:', error);
      setIsAddingNewCard(true);
    }
  };

  const saveCards = async (cards) => {
    await AsyncStorage.setItem(SAVED_CARDS_KEY, JSON.stringify(cards));
    setSavedCards(cards);
  };

  const removeSavedCard = async (cardId) => {
    const updatedCards = savedCards.filter((card) => card.id !== cardId);

    await saveCards(updatedCards);

    if (selectedCardId === cardId) {
      if (updatedCards.length > 0) {
        setSelectedCardId(updatedCards[0].id);
      } else {
        setSelectedCardId(null);
        setIsAddingNewCard(true);
      }
    }

    setMessage('Saved card removed.');
  };

  const getCardBrand = (number) => {
    const cleanNumber = number.replace(/\s/g, '');

    if (cleanNumber.startsWith('4')) return 'Visa';
    if (cleanNumber.startsWith('5')) return 'Mastercard';

    return 'Bank Card';
  };

  const saveNewCardLocally = async () => {
    const cleanNumber = cardNumber.replace(/\s/g, '');
    const last4 = cleanNumber.slice(-4);

    const newCard = {
      id: `${Date.now()}`,
      holder: cardHolder.trim(),
      last4,
      expiry: expiryDate.trim(),
      brand: getCardBrand(cleanNumber),
    };

    const updatedCards = [newCard, ...savedCards];

    await saveCards(updatedCards);
    setSelectedCardId(newCard.id);
    setIsAddingNewCard(false);

    return newCard;
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

  const clearNewCardFields = () => {
    setCardHolder('');
    setCardNumber('');
    setExpiryDate('');
    setCvv('');
    setRememberCard(true);
  };

  const validateAmount = () => {
    const topUpAmount = Number(amount);

    if (!topUpAmount || topUpAmount <= 0) {
      setMessage('Please enter a valid top-up amount.');
      return null;
    }

    return topUpAmount;
  };

  const validateNewCard = () => {
    if (!cardHolder.trim()) {
      setMessage('Please enter the card holder name.');
      return false;
    }

    if (cardNumber.replace(/\s/g, '').length < 16) {
      setMessage('Please enter a valid card number.');
      return false;
    }

    if (!expiryDate.trim()) {
      setMessage('Please enter the expiry date.');
      return false;
    }

    if (cvv.length < 3) {
      setMessage('Please enter a valid CVV.');
      return false;
    }

    return true;
  };

  const completePayment = (topUpAmount) => {
    setStatus('processing');
    setMessage('Processing payment...');

    setTimeout(() => {
      onPaymentSuccess?.(topUpAmount, 'Bank Card');

      setStatus('success');
      setMessage(
        `Payment successful. ${topUpAmount.toFixed(2)} EGP added to your balance.`
      );
    }, 1800);
  };

  const handlePay = async () => {
    const topUpAmount = validateAmount();
    if (!topUpAmount) return;

    if (!isAddingNewCard && selectedCardId) {
      completePayment(topUpAmount);
      return;
    }

    if (!validateNewCard()) return;

    setStatus('processing');
    setMessage('Processing payment...');

    setTimeout(async () => {
      if (rememberCard) {
        await saveNewCardLocally();
      }

      onPaymentSuccess?.(topUpAmount, 'Bank Card');

      setStatus('success');
      setMessage(
        `Payment successful. ${topUpAmount.toFixed(2)} EGP added to your balance.`
      );

      clearNewCardFields();
    }, 1800);
  };

  const selectedCard = savedCards.find((card) => card.id === selectedCardId);

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.paymentPageContent}>
        <TouchableOpacity style={styles.paymentBackButton} onPress={onBack}>
          <Text style={styles.paymentBackText}>← Back</Text>
        </TouchableOpacity>

        <View style={styles.cardVisual}>
          <Text style={styles.cardVisualLabel}>
            {!isAddingNewCard && selectedCard ? selectedCard.brand : 'Smart Meter Card'}
          </Text>

          <Text style={styles.cardVisualNumber}>
            {!isAddingNewCard && selectedCard
              ? `•••• •••• •••• ${selectedCard.last4}`
              : cardNumber || '4242 4242 4242 4242'}
          </Text>

          <View style={styles.cardVisualBottom}>
            <View>
              <Text style={styles.cardVisualSmallLabel}>Card Holder</Text>
              <Text style={styles.cardVisualValue}>
                {!isAddingNewCard && selectedCard
                  ? selectedCard.holder
                  : cardHolder || 'YOUR NAME'}
              </Text>
            </View>

            <View>
              <Text style={styles.cardVisualSmallLabel}>Expires</Text>
              <Text style={styles.cardVisualValue}>
                {!isAddingNewCard && selectedCard
                  ? selectedCard.expiry
                  : expiryDate || '12/28'}
              </Text>
            </View>
          </View>
        </View>

        <View style={styles.paymentFormCard}>
          <Text style={styles.paymentPageTitle}>Bank Card Payment</Text>
          <Text style={styles.paymentPageSubtitle}>
            Select a saved card or add a new card to complete the top-up.
          </Text>

          {savedCards.length > 0 && (
            <View style={styles.savedCardsSection}>
              <View style={styles.savedCardsHeader}>
                <Text style={styles.savedCardsTitle}>Saved Cards</Text>

                <TouchableOpacity
                  onPress={() => {
                    setIsAddingNewCard(true);
                    setSelectedCardId(null);
                    setMessage('');
                  }}
                >
                  <Text style={styles.addNewCardText}>Add New</Text>
                </TouchableOpacity>
              </View>

              {savedCards.map((card) => {
                const isSelected = selectedCardId === card.id && !isAddingNewCard;

                return (
                  <TouchableOpacity
                    key={card.id}
                    style={[
                      styles.savedCardOption,
                      isSelected && styles.savedCardOptionActive,
                    ]}
                    activeOpacity={0.85}
                    onPress={() => {
                      setSelectedCardId(card.id);
                      setIsAddingNewCard(false);
                      setMessage('');
                    }}
                  >
                    <View style={styles.savedCardOptionLeft}>
                      <View
                        style={[
                          styles.savedCardRadio,
                          isSelected && styles.savedCardRadioActive,
                        ]}
                      >
                        {isSelected && (
                          <Ionicons name="checkmark" size={13} color="#ffffff" />
                        )}
                      </View>

                      <View>
                        <Text style={styles.savedCardTitle}>
                          {card.brand} •••• {card.last4}
                        </Text>
                        <Text style={styles.savedCardSubtitle}>
                          {card.holder} · Expires {card.expiry}
                        </Text>
                      </View>
                    </View>

                    <TouchableOpacity onPress={() => removeSavedCard(card.id)}>
                      <Ionicons name="trash-outline" size={18} color="#dc2626" />
                    </TouchableOpacity>
                  </TouchableOpacity>
                );
              })}
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

          {(isAddingNewCard || savedCards.length === 0) && (
            <>
              <Text style={styles.inputLabel}>Card Holder Name</Text>
              <TextInput
                style={styles.input}
                value={cardHolder}
                onChangeText={setCardHolder}
                placeholder="Card holder name"
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
                  Save this card for future payments
                </Text>
              </TouchableOpacity>

              {savedCards.length > 0 && (
                <TouchableOpacity
                  style={styles.cancelNewCardButton}
                  onPress={() => {
                    setIsAddingNewCard(false);
                    setSelectedCardId(savedCards[0]?.id ?? null);
                    clearNewCardFields();
                    setMessage('');
                  }}
                >
                  <Text style={styles.cancelNewCardText}>Cancel</Text>
                </TouchableOpacity>
              )}
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