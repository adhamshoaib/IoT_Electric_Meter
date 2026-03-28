import React, { useState, useCallback } from 'react';
import {
  StyleSheet,
  Text,
  View,
  ScrollView,
  SafeAreaView,
  TouchableOpacity,
  StatusBar,
  RefreshControl,
  Dimensions,
} from 'react-native';
import { Svg, Circle, Defs, LinearGradient, Stop } from 'react-native-svg';
import { LucideBolt, LucideWallet, LucideChevronRight, LucideLayoutGrid, LucideBarChart3, LucideRouter, LucideSettings } from 'lucide-react-native';
import { LinearGradient as ExpoGradient } from 'expo-linear-gradient';

const { width } = Dimensions.get('window');
const CIRCLE_SIZE = width * 0.7;
const STROKE_WIDTH = 15;
const RADIUS = (CIRCLE_SIZE - STROKE_WIDTH) / 2;
const CIRCUMFERENCE = 2 * Math.PI * RADIUS;

export default function DashboardScreen() {
  const [refreshing, setRefreshing] = useState(false);
  const [consumption, setConsumption] = useState(482);
  const [balance, setBalance] = useState(458.00);

  const onRefresh = useCallback(() => {
    setRefreshing(true);
    setTimeout(() => {
      setConsumption(Math.floor(Math.random() * 100) + 400);
      setBalance(prev => prev - 5);
      setRefreshing(false);
    }, 2000);
  }, []);

  const progress = 0.65;
  const strokeDashoffset = CIRCUMFERENCE - (CIRCUMFERENCE * progress);

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <View style={styles.header}>
        <View style={styles.headerTitleContainer}>
          <LucideBolt color="#00d2c8" size={24} />
          <Text style={styles.headerTitle}>ENERGY PULSE</Text>
        </View>
        <View style={styles.avatarPlaceholder} />
      </View>

      <ScrollView
        contentContainerStyle={styles.scrollContent}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#00d2c8" />
        }
      >
        {/* Main Consumption Gauge */}
        <View style={styles.gaugeContainer}>
          <Svg width={CIRCLE_SIZE} height={CIRCLE_SIZE} style={styles.svg}>
            <Defs>
              <LinearGradient id="tealGradient" x1="0%" y1="0%" x2="100%" y2="0%">
                <Stop offset="0%" stopColor="#00d2c8" />
                <Stop offset="100%" stopColor="#00f2e8" />
              </LinearGradient>
            </Defs>
            <Circle
              cx={CIRCLE_SIZE / 2}
              cy={CIRCLE_SIZE / 2}
              r={RADIUS}
              stroke="#0f1623"
              strokeWidth={STROKE_WIDTH}
              fill="transparent"
            />
            <Circle
              cx={CIRCLE_SIZE / 2}
              cy={CIRCLE_SIZE / 2}
              r={RADIUS}
              stroke="url(#tealGradient)"
              strokeWidth={STROKE_WIDTH}
              fill="transparent"
              strokeDasharray={CIRCUMFERENCE}
              strokeDashoffset={strokeDashoffset}
              strokeLinecap="round"
              transform={`rotate(-90, ${CIRCLE_SIZE / 2}, ${CIRCLE_SIZE / 2})`}
            />
          </Svg>
          <View style={styles.gaugeTextContainer}>
            <Text style={styles.gaugeLabel}>CURRENT MONTH</Text>
            <Text style={styles.gaugeValue}>{consumption}</Text>
            <Text style={styles.gaugeUnit}>kWh</Text>
            <View style={styles.trendBadge}>
              <Text style={styles.trendText}>↗ 12% ABOVE AVERAGE</Text>
            </View>
          </View>
        </View>

        {/* Cost Display */}
        <View style={styles.costContainer}>
          <Text style={styles.costLabel}>Estimated Monthly Cost</Text>
          <Text style={styles.costValue}>1,245.50 <Text style={styles.currency}>EGP</Text></Text>
        </View>

        {/* Balance Card */}
        <View style={styles.card}>
          <View>
            <Text style={styles.cardLabel}>PREPAID BALANCE</Text>
            <Text style={styles.cardValue}>{balance.toFixed(2)} <Text style={styles.currency}>EGP</Text></Text>
          </View>
          <TouchableOpacity activeOpacity={0.8}>
            <ExpoGradient
              colors={['#00d2c8', '#00a39b']}
              style={styles.topUpButton}
              start={{ x: 0, y: 0 }}
              end={{ x: 1, y: 1 }}
            >
              <Text style={styles.topUpText}>+ TOP UP</Text>
            </ExpoGradient>
          </TouchableOpacity>
        </View>

        {/* Secondary Info Grid */}
        <View style={styles.grid}>
          <View style={[styles.card, styles.gridItem]}>
            <LucideBolt color="#00d2c8" size={20} />
            <Text style={styles.gridLabel}>Current Load</Text>
            <Text style={styles.gridValue}>2.4 <Text style={styles.gridUnit}>kW</Text></Text>
          </View>
          <View style={[styles.card, styles.gridItem]}>
            <LucideWallet color="#00d2c8" size={20} />
            <Text style={styles.gridLabel}>Billing Cycle</Text>
            <Text style={styles.gridValue}>12 <Text style={styles.gridUnit}>Days Left</Text></Text>
          </View>
        </View>

        {/* Action Button */}
        <TouchableOpacity style={styles.fullWidthButton} activeOpacity={0.7}>
          <View style={styles.buttonContent}>
            <LucideBarChart3 color="#f8fafc" size={20} />
            <Text style={styles.buttonText}>View Detailed Statistics</Text>
          </View>
          <LucideChevronRight color="#94a3b8" size={20} />
        </TouchableOpacity>

        <View style={{ height: 100 }} />
      </ScrollView>

      {/* Navigation Bar */}
      <View style={styles.navBar}>
        <TouchableOpacity style={styles.navItem}>
          <View style={styles.activeNavCircle}>
            <LucideLayoutGrid color="#00d2c8" size={24} />
          </View>
          <Text style={styles.activeNavText}>DASHBOARD</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navItem}>
          <LucideBarChart3 color="#94a3b8" size={24} />
          <Text style={styles.navText}>STATS</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navItem}>
          <LucideRouter color="#94a3b8" size={24} />
          <Text style={styles.navText}>DEVICES</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navItem}>
          <LucideSettings color="#94a3b8" size={24} />
          <Text style={styles.navText}>SETTINGS</Text>
        </TouchableOpacity>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#080d14',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 24,
    paddingVertical: 16,
  },
  headerTitleContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  headerTitle: {
    color: '#00d2c8',
    fontSize: 18,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  avatarPlaceholder: {
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: '#0f1623',
    borderWidth: 1,
    borderColor: '#1a2233',
  },
  scrollContent: {
    paddingHorizontal: 20,
    paddingBottom: 100,
  },
  gaugeContainer: {
    alignItems: 'center',
    marginVertical: 40,
  },
  svg: {
    shadowColor: '#00d2c8',
    shadowOffset: { width: 0, height: 0 },
    shadowOpacity: 0.2,
    shadowRadius: 20,
  },
  gaugeTextContainer: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    justifyContent: 'center',
    alignItems: 'center',
  },
  gaugeLabel: {
    color: '#94a3b8',
    fontSize: 12,
    fontWeight: '600',
    letterSpacing: 1,
  },
  gaugeValue: {
    color: '#f8fafc',
    fontSize: 64,
    fontWeight: 'bold',
  },
  gaugeUnit: {
    color: '#00d2c8',
    fontSize: 20,
    fontWeight: '600',
    marginTop: -8,
  },
  trendBadge: {
    backgroundColor: 'rgba(0, 210, 200, 0.1)',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 20,
    marginTop: 16,
    borderWidth: 1,
    borderColor: 'rgba(0, 210, 200, 0.2)',
  },
  trendText: {
    color: '#00d2c8',
    fontSize: 10,
    fontWeight: 'bold',
  },
  costContainer: {
    alignItems: 'center',
    marginBottom: 40,
  },
  costLabel: {
    color: '#94a3b8',
    fontSize: 14,
    marginBottom: 8,
  },
  costValue: {
    color: '#f8fafc',
    fontSize: 32,
    fontWeight: '700',
  },
  currency: {
    fontSize: 16,
    color: '#94a3b8',
    fontWeight: '400',
  },
  card: {
    backgroundColor: '#0f1623',
    borderRadius: 24,
    padding: 24,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#1a2233',
  },
  cardLabel: {
    color: '#94a3b8',
    fontSize: 12,
    fontWeight: '700',
    letterSpacing: 1,
    marginBottom: 8,
  },
  cardValue: {
    color: '#f8fafc',
    fontSize: 28,
    fontWeight: '700',
  },
  topUpButton: {
    paddingHorizontal: 20,
    paddingVertical: 14,
    borderRadius: 16,
    shadowColor: '#00d2c8',
    shadowOffset: { width: 0, height: 4 },
    shadowOpacity: 0.3,
    shadowRadius: 8,
  },
  topUpText: {
    color: '#080d14',
    fontSize: 14,
    fontWeight: '900',
  },
  grid: {
    flexDirection: 'row',
    gap: 16,
  },
  gridItem: {
    flex: 1,
    flexDirection: 'column',
    alignItems: 'flex-start',
    gap: 12,
  },
  gridLabel: {
    color: '#94a3b8',
    fontSize: 12,
  },
  gridValue: {
    color: '#f8fafc',
    fontSize: 20,
    fontWeight: '700',
  },
  gridUnit: {
    fontSize: 12,
    color: '#94a3b8',
  },
  fullWidthButton: {
    backgroundColor: '#0f1623',
    padding: 20,
    borderRadius: 16,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 16,
    borderWidth: 1,
    borderColor: '#1a2233',
  },
  buttonContent: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  buttonText: {
    color: '#f8fafc',
    fontSize: 16,
    fontWeight: '600',
  },
  navBar: {
    position: 'absolute',
    bottom: 0,
    left: 0,
    right: 0,
    backgroundColor: 'rgba(15, 22, 35, 0.95)',
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingTop: 12,
    paddingBottom: 34,
    borderTopWidth: 1,
    borderTopColor: '#1a2233',
  },
  navItem: {
    alignItems: 'center',
    gap: 6,
  },
  activeNavCircle: {
    backgroundColor: 'rgba(0, 210, 200, 0.1)',
    padding: 8,
    borderRadius: 12,
  },
  activeNavText: {
    color: '#00d2c8',
    fontSize: 10,
    fontWeight: 'bold',
  },
  navText: {
    color: '#94a3b8',
    fontSize: 10,
    fontWeight: '500',
  },
});
