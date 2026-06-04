import React, { useEffect, useState } from 'react';
import {
  View,
  Text,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import styles from '../styles';
import { calculateEgyptBill } from '../services/calculateEgyptBill';

export default function DashboardScreen({ user, data, onOpenSettings }) {
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

const isMeterOnline = readingAgeSeconds <= 180;
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