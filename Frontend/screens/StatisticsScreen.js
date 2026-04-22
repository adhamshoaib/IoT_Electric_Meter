import React from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  Dimensions,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { BarChart } from 'react-native-chart-kit';

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

const lastWeekLabels = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
const lastWeekKwh = [12.4, 10.8, 13.1, 11.5, 14.2, 16.8, 15.3];

const previousWeekKwh = [10.9, 10.2, 11.8, 10.7, 12.6, 14.9, 13.8];

export default function StatisticsScreen({ onBack }) {
  const weekTotalKwh = lastWeekKwh.reduce((a, b) => a + b, 0);
  const previousWeekTotalKwh = previousWeekKwh.reduce((a, b) => a + b, 0);
  const weekTotalCost = calculateCost(weekTotalKwh);
  const weekAvgKwh = weekTotalKwh / 7;
  const weekMaxKwh = Math.max(...lastWeekKwh);
  const weekMinKwh = Math.min(...lastWeekKwh);
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
                {weekTotalKwh.toFixed(1)}
                <Text style={styles.heroUnit}> kWh</Text>
              </Text>
              <Text style={styles.heroSubtext}>Estimated cost: EGP {weekTotalCost.toFixed(2)}</Text>
            </View>

            <View style={styles.heroPill}>
              <Text style={styles.heroPillText}>{trendText}</Text>
            </View>
          </View>

          <View style={styles.heroDivider} />

          <View style={styles.heroBottomRow}>
            <View style={styles.heroBottomItem}>
              <Text style={styles.heroBottomLabel}>Average / Day</Text>
              <Text style={styles.heroBottomValue}>{weekAvgKwh.toFixed(1)} kWh</Text>
            </View>

            <View style={styles.heroBottomItem}>
              <Text style={styles.heroBottomLabel}>Peak Day</Text>
              <Text style={styles.heroBottomValue}>{weekMaxDay}</Text>
            </View>

            <View style={styles.heroBottomItem}>
              <Text style={styles.heroBottomLabel}>Highest Reading</Text>
              <Text style={styles.heroBottomValue}>{weekMaxKwh.toFixed(1)} kWh</Text>
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
              <Text style={styles.comparisonValue}>{weekTotalKwh.toFixed(1)} kWh</Text>
            </View>

            <View style={styles.comparisonDivider} />

            <View style={styles.comparisonItem}>
              <Text style={styles.comparisonLabel}>Last Week</Text>
              <Text style={styles.comparisonValue}>{previousWeekTotalKwh.toFixed(1)} kWh</Text>
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
            <BarChart
              data={{ labels: lastWeekLabels, datasets: [{ data: lastWeekKwh }] }}
              width={screenWidth + 160}
              height={220}
              chartConfig={chartConfig}
              style={styles.chart}
              withInnerLines={false}
              showValuesOnTopOfBars
              segments={4}
              fromZero
            />
          </ScrollView>
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
    color: '#0f766e',
  },
  downValue: {
    color: '#c2410c',
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