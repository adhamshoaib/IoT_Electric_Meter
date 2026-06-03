import React, { useEffect, useState } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  Dimensions,
  TouchableOpacity,
  ActivityIndicator,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { BarChart } from 'react-native-chart-kit';
import { ref, onValue } from 'firebase/database';
import { database } from '../services/firebase';

const screenWidth = Dimensions.get('window').width;

const chartConfig = {
  backgroundGradientFrom: '#ffffff',
  backgroundGradientTo: '#ffffff',
  color: (opacity = 1) => `rgba(15, 118, 110, ${opacity})`,
  labelColor: (opacity = 1) => `rgba(100, 116, 139, ${opacity})`,
  strokeWidth: 2.5,
  propsForDots: {
    r: '4',
    strokeWidth: '0',
    stroke: '#0f766e',
  },
  propsForBackgroundLines: {
    stroke: '#e2e8f0',
  },
};

const calculateCost = (kwh) => {
  if (kwh <= 50) return kwh * 0.58;
  else if (kwh <= 100) return 50 * 0.58 + (kwh - 50) * 0.68;
  else if (kwh <= 200) return 50 * 0.58 + 50 * 0.68 + (kwh - 100) * 0.83;
  else if (kwh <= 350) {
    return 50 * 0.58 + 50 * 0.68 + 100 * 0.83 + (kwh - 200) * 1.25;
  } else if (kwh <= 650) {
    return (
      50 * 0.58 +
      50 * 0.68 +
      100 * 0.83 +
      150 * 1.25 +
      (kwh - 350) * 1.4
    );
  } else if (kwh <= 1000) {
    return (
      50 * 0.58 +
      50 * 0.68 +
      100 * 0.83 +
      150 * 1.25 +
      300 * 1.4 +
      (kwh - 650) * 1.5
    );
  } else {
    return (
      50 * 0.58 +
      50 * 0.68 +
      100 * 0.83 +
      150 * 1.25 +
      300 * 1.4 +
      350 * 1.5 +
      (kwh - 1000) * 1.65
    );
  }
};

const DAY_LABELS = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
//const lastWeekKwh = [12.4, 10.8, 13.1, 11.5, 14.2, 16.8, 15.3];

//const previousWeekKwh = [10.9, 10.2, 11.8, 10.7, 12.6, 14.9, 13.8];

export default function StatisticsScreen({ onBack }) {

const [lastWeekKwh, setLastWeekKwh] = useState([0,0,0,0,0,0,0]);
const [lastWeekLabels, setLastWeekLabels] = useState(['D-6','D-5','D-4','D-3','D-2','D-1','Today']);
const [previousWeekKwh, setPreviousWeekKwh] = useState([0,0,0,0,0,0,0]);
const [monthlyHistory, setMonthlyHistory] = useState([]);
const [isLoading, setIsLoading] = useState(true);

const START_TS = 1779991488;

// Groups logs by calendar day and returns sorted array of { dateKey, kwh, dayIndex, ts }
const buildDailyConsumption = (readings) => {
  const sorted = [...readings]
    .filter(r => r.ts != null && r.energy_kwh != null && r.ts >= START_TS)
    .sort((a, b) => a.ts - b.ts);

  // Group by calendar date key YYYY-MM-DD
  const byDay = {};
  sorted.forEach(r => {
    const d = new Date(r.ts * 1000);
    const key = `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`;
    if (!byDay[key]) byDay[key] = [];
    byDay[key].push(r);
  });

  // Each day = last log of day - first log of day
  return Object.keys(byDay).sort().map(key => {
    const logs = byDay[key];
// Each day = last reading of the day - last reading of the previous day
const previousDayLastReading =
  sorted.findLast(r => r.ts < logs[0].ts)?.energy_kwh ?? logs[0].energy_kwh;

const currentDayLastReading = logs[logs.length - 1].energy_kwh;

const kwh = Math.max(0, currentDayLastReading - previousDayLastReading);
    const d = new Date(logs[0].ts * 1000);
    return { dateKey: key, kwh, dayIndex: d.getDay(), ts: logs[0].ts };
  });
};

const processWeeklyData = (readings) => {
  const days = buildDailyConsumption(readings);

  if (days.length === 0) {
    setLastWeekKwh([0]);
    setPreviousWeekKwh([0]);
    setLastWeekLabels(['Today']);
    return;
  }

  // Last 7 days = this week, 7 before that = previous week
  const last7 = days.slice(-7);
  const prev7 = days.slice(-14, -7);

  // Labels from real day names, rightmost = Today
  const labels = last7.map((d, i) =>
    i === last7.length - 1 ? 'Today' : DAY_LABELS[d.dayIndex]
  );

  setLastWeekLabels(labels);
  setLastWeekKwh(last7.map(d => d.kwh));
  setPreviousWeekKwh(prev7.map(d => d.kwh));
};

const processMonthlyData = (readings) => {
  const days = buildDailyConsumption(readings);

  if (days.length === 0) {
    setMonthlyHistory([]);
    return;
  }

  // Group daily consumptions by calendar month
  const byMonth = {};
  days.forEach(d => {
    const [y, m] = d.dateKey.split('-');
    const key = `${y}-${m}`;
    if (!byMonth[key]) byMonth[key] = 0;
    byMonth[key] += d.kwh;
  });

  // Sort newest first and build result
  const result = Object.keys(byMonth).sort().reverse().slice(0, 6).map(key => {
    const [y, m] = key.split('-').map(Number);
    const label = new Date(y, m - 1, 1).toLocaleString('default', { month: 'long', year: 'numeric' });
    return { label, kwh: byMonth[key] };
  });

  setMonthlyHistory(result);
};

useEffect(() => {
  const historyRef = ref(database, 'logs');

  const unsubscribe = onValue(historyRef, (snapshot) => {
    const data = snapshot.val();
    if (!data) {
      setIsLoading(false);
      return;
    }
    const readings = Object.values(data);
    console.log('Total logs:', readings.length);
    console.log('Sample:', readings.slice(-3));
    processWeeklyData(readings);
    processMonthlyData(readings);
    setIsLoading(false);
  });

  return () => unsubscribe();
}, []);

  const weekTotalKwh = lastWeekKwh.reduce((a, b) => a + b, 0);
  const previousWeekTotalKwh = previousWeekKwh.reduce((a, b) => a + b, 0);
  const weekTotalCost = calculateCost(weekTotalKwh);
  // FIX: divide by actual days with data, not always 7
  const daysWithData = lastWeekKwh.filter(v => v > 0).length || 1;
  const weekAvgKwh = weekTotalKwh / daysWithData;
  const weekMaxKwh = Math.max(...lastWeekKwh);
  const weekMaxDay = lastWeekLabels[lastWeekKwh.indexOf(weekMaxKwh)];

  const weeklyChangePercent =
    previousWeekTotalKwh === 0
      ? 0
      : ((weekTotalKwh - previousWeekTotalKwh) / previousWeekTotalKwh) * 100;

  const projectedMonthlyKwh = Math.round(weekAvgKwh * 30);
  const projectedMonthlyCost = calculateCost(projectedMonthlyKwh);

  const trendText =
    weeklyChangePercent > 0
      ? `Up ${weeklyChangePercent.toFixed(1)}% from last week`
      : weeklyChangePercent < 0
      ? `Down ${Math.abs(weeklyChangePercent).toFixed(1)}% from last week`
      : 'Same as last week';

  return (
    <View style={styles.container}>
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <View style={styles.headerTextWrap}>
          <Text style={styles.pageTitle}>Statistics</Text>
          <Text style={styles.pageSubtitle}>Track your recent energy trend</Text>
        </View>

        <View style={styles.topSpacer} />
      </View>

      <ScrollView
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={false}
      >
        <View style={styles.heroCard}>
          <View style={styles.heroGlowOne} />
          <View style={styles.heroGlowTwo} />

          <View style={styles.heroTopRow}>
            <View>
              <Text style={styles.heroEyebrow}>THIS WEEK</Text>
              <Text style={styles.heroMainValue}>
                {weekTotalKwh.toFixed(4)}
                <Text style={styles.heroUnit}> kWh</Text>
              </Text>
              <Text style={styles.heroSubtext}>Estimated cost: EGP {weekTotalCost.toFixed(2)}</Text>
            </View>

            {weeklyChangePercent !== 0 && (
            <View style={styles.heroPill}>
              <Text style={styles.heroPillText}>{trendText}</Text>
            </View>
            )}
          </View>

          <View style={styles.heroDivider} />

          <View style={styles.heroBottomRow}>
            <View style={styles.heroBottomItem}>
              <Text style={styles.heroBottomLabel}>Average / Day</Text>
              <Text style={styles.heroBottomValue}>{weekAvgKwh.toFixed(4)} kWh</Text>
            </View>

            <View style={styles.heroBottomItem}>
              <Text style={styles.heroBottomLabel}>Peak Day</Text>
              <Text style={styles.heroBottomValue}>{weekMaxDay}</Text>
            </View>
          </View>
        </View>

        <View style={styles.comparisonCard}>
          <View style={styles.comparisonHeader}>
            <Text style={styles.sectionTitle}>Weekly Comparison</Text>
            <Text style={styles.sectionSubtext}>How this week compares to the previous one</Text>
          </View>

          <View style={styles.comparisonRow}>
            <View style={styles.comparisonItem}>
              <Text style={styles.comparisonLabel}>This Week</Text>
              <Text style={styles.comparisonValue}>{weekTotalKwh.toFixed(4)} kWh</Text>
            </View>

            <View style={styles.comparisonDivider} />

            <View style={styles.comparisonItem}>
              <Text style={styles.comparisonLabel}>Last Week</Text>
              <Text style={styles.comparisonValue}>{previousWeekTotalKwh.toFixed(4)} kWh</Text>
            </View>

            <View style={styles.comparisonDivider} />

            <View style={styles.comparisonItem}>
              <Text style={styles.comparisonLabel}>Change</Text>
              <Text
                style={[
                  styles.comparisonValue,
                  weeklyChangePercent >= 0 ? styles.upValue : styles.downValue,
                ]}
              >
                {weeklyChangePercent >= 0 ? '+' : ''}
                {weeklyChangePercent.toFixed(1)}%
              </Text>
            </View>
          </View>
        </View>

        <View style={styles.chartCard}>
          <View style={styles.chartHeader}>
            <View>
              <Text style={styles.sectionTitle}>Usage Trend</Text>
              <Text style={styles.sectionSubtext}>
                Daily electricity usage over the last 7 days
              </Text>
            </View>

            <View style={styles.chartBadge}>
              <Text style={styles.chartBadgeText}>Last 7 days</Text>
            </View>
          </View>

          <ScrollView
            horizontal
            showsHorizontalScrollIndicator={false}
            contentContainerStyle={styles.chartScrollContent}
          >
            {isLoading ? (
              <View style={styles.chartLoader}>
                <ActivityIndicator size="small" color="#0f766e" />
                <Text style={styles.chartLoaderText}>Loading chart...</Text>
              </View>
            ) : (
              <BarChart
                data={{ labels: lastWeekLabels, datasets: [{ data: lastWeekKwh.length ? lastWeekKwh.map(v => parseFloat(v.toFixed(4))) : [0] }] }}
                width={screenWidth + 160}
                height={220}
                chartConfig={chartConfig}
                style={styles.chart}
                withInnerLines={false}
                showValuesOnTopOfBars
                segments={4}
                fromZero
              />
            )}
          </ScrollView>
        </View>

        {/* ── Monthly History Card ── */}
        <View style={styles.monthlyCard}>
          <View style={styles.monthlyHeader}>
            <View style={styles.projectionIconWrap}>
              <Ionicons name="calendar-outline" size={18} color="#0f766e" />
            </View>
            <View style={styles.projectionTextWrap}>
              <Text style={styles.sectionTitle}>Previous Months</Text>
              <Text style={styles.sectionSubtext}>Last 6 months from your meter</Text>
            </View>
          </View>

          {isLoading ? (
            <ActivityIndicator size="small" color="#0f766e" style={{ marginVertical: 16 }} />
          ) : monthlyHistory.length === 0 ? (
            <Text style={styles.monthlyEmpty}>No historical data available yet.</Text>
          ) : (
            monthlyHistory.map((item, index) => {
              const cost = calculateCost(item.kwh);
              const isLast = index === monthlyHistory.length - 1;
              return (
                <View key={index} style={[styles.monthlyRow, isLast && styles.monthlyRowLast]}>
                  <View style={styles.monthlyDot} />
                  <View style={styles.monthlyInfo}>
                    <Text style={styles.monthlyName}>{item.label}</Text>
                  </View>
                  <View style={styles.monthlyValues}>
                    <Text style={styles.monthlyKwh}>{item.kwh.toFixed(4)} kWh</Text>
                    <Text style={styles.monthlyCost}>EGP {cost.toFixed(2)}</Text>
                  </View>
                </View>
              );
            })
          )}
        </View>

        <View style={styles.projectionCard}>
          <View style={styles.projectionHeader}>
            <View style={styles.projectionIconWrap}>
              <Ionicons name="flash-outline" size={18} color="#0f766e" />
            </View>

            <View style={styles.projectionTextWrap}>
              <Text style={styles.sectionTitle}>Monthly Projection</Text>
              <Text style={styles.sectionSubtext}>
                Based on your current weekly average
              </Text>
            </View>
          </View>

          <View style={styles.projectionRow}>
            <View style={styles.projectionItem}>
              <Text style={styles.projectionLabel}>Projected Usage</Text>
              <Text style={styles.projectionValue}>{projectedMonthlyKwh} kWh</Text>
            </View>

            <View style={styles.projectionItem}>
              <Text style={styles.projectionLabel}>Projected Cost</Text>
              <Text style={styles.projectionValue}>EGP {projectedMonthlyCost.toFixed(2)}</Text>
            </View>
          </View>

          <Text style={styles.projectionFootnote}>
            If your current pattern continues, your end-of-month consumption may reach about{' '}
            {projectedMonthlyKwh} kWh.
          </Text>
        </View>

        <View style={{ height: 12 }} />
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f4f7fb',
  },

  pageTopRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 20,
    paddingTop: 16,
    paddingBottom: 14,
    borderBottomWidth: 1,
    borderBottomColor: '#e2e8f0',
    backgroundColor: '#f4f7fb',
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
  headerTextWrap: {
    alignItems: 'center',
  },
  pageTitle: {
    fontSize: 24,
    fontWeight: '800',
    color: '#0f172a',
  },
  pageSubtitle: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 2,
  },
  topSpacer: {
    width: 42,
  },

  scrollContent: {
    padding: 20,
    paddingBottom: 110,
  },

  heroCard: {
    backgroundColor: '#0f766e',
    borderRadius: 28,
    padding: 22,
    marginBottom: 18,
    overflow: 'hidden',
    shadowColor: '#0f766e',
    shadowOpacity: 0.18,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 5,
  },
  heroGlowOne: {
    position: 'absolute',
    width: 170,
    height: 170,
    borderRadius: 85,
    backgroundColor: '#14b8a6',
    top: -55,
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
  heroTopRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
  },
  heroEyebrow: {
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1,
    color: '#ccfbf1',
    marginBottom: 10,
  },
  heroMainValue: {
    fontSize: 34,
    fontWeight: '900',
    color: '#ffffff',
  },
  heroUnit: {
    fontSize: 18,
    fontWeight: '700',
    color: '#d1fae5',
  },
  heroSubtext: {
    fontSize: 14,
    color: '#d1fae5',
    marginTop: 6,
  },
  heroPill: {
    backgroundColor: 'rgba(255,255,255,0.16)',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 999,
    maxWidth: 140,
  },
  heroPillText: {
    color: '#ffffff',
    fontSize: 12,
    fontWeight: '700',
    textAlign: 'center',
  },
  heroDivider: {
    height: 1,
    backgroundColor: 'rgba(255,255,255,0.18)',
    marginVertical: 18,
  },
  heroBottomRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    gap: 12,
  },
  heroBottomItem: {
    flex: 1,
  },
  heroBottomLabel: {
    fontSize: 12,
    color: '#ccfbf1',
    marginBottom: 6,
    fontWeight: '600',
  },
  heroBottomValue: {
    fontSize: 16,
    fontWeight: '800',
    color: '#ffffff',
  },

  comparisonCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    marginBottom: 18,
    borderWidth: 1,
    borderColor: '#e2e8f0',
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  comparisonHeader: {
    marginBottom: 14,
  },
  comparisonRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  comparisonItem: {
    flex: 1,
  },
  comparisonDivider: {
    width: 1,
    height: 42,
    backgroundColor: '#e2e8f0',
    marginHorizontal: 12,
  },
  comparisonLabel: {
    fontSize: 12,
    color: '#64748b',
    marginBottom: 8,
    fontWeight: '600',
  },
  comparisonValue: {
    fontSize: 17,
    fontWeight: '800',
    color: '#0f172a',
  },
  upValue: {
    color: '#c2410c', // red = bad, consuming more than last week
  },
  downValue: {
    color: '#0f766e', // green = good, consuming less than last week
  },

  chartCard: {
    backgroundColor: '#ffffff',
    borderWidth: 1,
    borderColor: '#e2e8f0',
    borderRadius: 24,
    paddingTop: 20,
    paddingLeft: 20,
    paddingRight: 20,
    paddingBottom: 20,
    marginBottom: 18,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  chartHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    marginBottom: 16,
  },
  chartBadge: {
    backgroundColor: '#ecfdf5',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 999,
  },
  chartBadgeText: {
    fontSize: 11,
    fontWeight: '700',
    color: '#0f766e',
  },
  chartScrollContent: {
    paddingRight: 24,
  },
  chart: {
    borderRadius: 12,
  },

  projectionCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    borderWidth: 1,
    borderColor: '#e2e8f0',
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  projectionHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  projectionIconWrap: {
    width: 38,
    height: 38,
    borderRadius: 19,
    backgroundColor: '#ecfdf5',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  projectionTextWrap: {
    flex: 1,
  },
  projectionRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    gap: 12,
    marginBottom: 14,
  },
  projectionItem: {
    flex: 1,
    backgroundColor: '#f8fafc',
    borderRadius: 18,
    padding: 14,
  },
  projectionLabel: {
    fontSize: 12,
    color: '#64748b',
    marginBottom: 8,
    fontWeight: '600',
  },
  projectionValue: {
    fontSize: 20,
    fontWeight: '800',
    color: '#0f172a',
  },
  projectionFootnote: {
    fontSize: 13,
    lineHeight: 19,
    color: '#64748b',
  },

  chartLoader: {
    width: screenWidth - 40,
    height: 220,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 10,
  },
  chartLoaderText: {
    fontSize: 13,
    color: '#64748b',
  },

  monthlyCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    marginBottom: 18,
    borderWidth: 1,
    borderColor: '#e2e8f0',
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  monthlyHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 18,
  },
  monthlyEmpty: {
    fontSize: 13,
    color: '#94a3b8',
    textAlign: 'center',
    paddingVertical: 12,
  },
  monthlyRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingBottom: 14,
    marginBottom: 14,
    borderBottomWidth: 1,
    borderBottomColor: '#f1f5f9',
  },
  monthlyRowLast: {
    borderBottomWidth: 0,
    marginBottom: 0,
    paddingBottom: 0,
  },
  monthlyDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    backgroundColor: '#0f766e',
    marginRight: 12,
  },
  monthlyInfo: {
    flex: 1,
  },
  monthlyName: {
    fontSize: 15,
    fontWeight: '700',
    color: '#0f172a',
  },
  monthlyValues: {
    alignItems: 'flex-end',
  },
  monthlyKwh: {
    fontSize: 15,
    fontWeight: '800',
    color: '#0f172a',
  },
  monthlyCost: {
    fontSize: 12,
    fontWeight: '600',
    color: '#64748b',
    marginTop: 2,
  },

  sectionTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
  },
  sectionSubtext: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 4,
    lineHeight: 18,
  },
});