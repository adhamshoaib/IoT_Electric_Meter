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
import { LineChart, BarChart } from 'react-native-chart-kit';

const screenWidth = Dimensions.get('window').width;

const chartConfig = {
  backgroundGradientFrom: '#ffffff',
  backgroundGradientTo: '#f8fafc',
  color: (opacity = 1) => `rgba(15, 118, 110, ${opacity})`,
  labelColor: (opacity = 1) => `rgba(100, 116, 139, ${opacity})`,
  strokeWidth: 2.5,
  propsForDots: { r: '4', strokeWidth: '0', stroke: '#0f766e' },
  propsForBackgroundLines: { stroke: '#e2e8f0' },
};

const calculateCost = (kwh) => {
  if (kwh <= 50) return kwh * 0.58;
  else if (kwh <= 100) return 50 * 0.58 + (kwh - 50) * 0.68;
  else if (kwh <= 200) return 50 * 0.58 + 50 * 0.68 + (kwh - 100) * 0.83;
  else if (kwh <= 350) return 50 * 0.58 + 50 * 0.68 + 100 * 0.83 + (kwh - 200) * 1.25;
  else if (kwh <= 650) return 50 * 0.58 + 50 * 0.68 + 100 * 0.83 + 150 * 1.25 + (kwh - 350) * 1.40;
  else if (kwh <= 1000) return 50 * 0.58 + 50 * 0.68 + 100 * 0.83 + 150 * 1.25 + 300 * 1.40 + (kwh - 650) * 1.50;
  else return 50 * 0.58 + 50 * 0.68 + 100 * 0.83 + 150 * 1.25 + 300 * 1.40 + 350 * 1.50 + (kwh - 1000) * 1.65;
};

const lastWeekLabels = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
const lastWeekKwh = [12.4, 10.8, 13.1, 11.5, 14.2, 16.8, 15.3];
const lastWeekCost = lastWeekKwh.map((kwh) => calculateCost(kwh));

const last6MonthsLabels = ['Mar', 'Feb', 'Jan', 'Dec', 'Nov', 'Oct'];
const last6MonthsKwh = [295, 315, 380, 340, 310, 315];
const last6MonthsCost = last6MonthsKwh.map((kwh) => calculateCost(kwh));

export default function StatisticsScreen({ onBack }) {
  const weekTotalKwh = lastWeekKwh.reduce((a, b) => a + b, 0);
  const weekTotalCost = lastWeekCost.reduce((a, b) => a + b, 0);
  const weekAvgKwh = (weekTotalKwh / 7).toFixed(1);
  const weekMaxKwh = Math.max(...lastWeekKwh);
  const weekMinKwh = Math.min(...lastWeekKwh);
  const weekMaxDay = lastWeekLabels[lastWeekKwh.indexOf(weekMaxKwh)];

  return (
    <View style={styles.container}>
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Statistics</Text>

        <View style={styles.topSpacer} />
      </View>

      <ScrollView style={styles.scroll} showsVerticalScrollIndicator={false}>
        <Text style={styles.sectionTitle}>Last Week</Text>

        <View style={styles.summaryRow}>
          <View style={styles.summaryCard}>
            <Text style={styles.summaryLabel}>Total Consumption</Text>
            <View style={styles.valueRow}>
              <Text style={styles.summaryValue}>{weekTotalKwh.toFixed(1)}</Text>
              <Text style={styles.summaryUnit}>kWh</Text>
            </View>
          </View>

          <View style={styles.summaryCard}>
            <Text style={styles.summaryLabel}>Total Cost</Text>
            <View style={styles.valueRow}>
              <Text style={[styles.summaryValue, { color: '#0f766e' }]}>
                EGP {weekTotalCost.toFixed(2)}
              </Text>
            </View>
          </View>
        </View>

        <View style={styles.statsGrid}>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Daily Avg</Text>
            <Text style={styles.statValue}>{weekAvgKwh} kWh</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Peak Day</Text>
            <Text style={styles.statValue}>{weekMaxDay}</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Lowest</Text>
            <Text style={styles.statValue}>{weekMinKwh} kWh</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Highest</Text>
            <Text style={styles.statValue}>{weekMaxKwh} kWh</Text>
          </View>
        </View>

        <View style={styles.chartCard}>
          <View style={styles.chartHeader}>
            <Text style={styles.chartTitle}>Daily Consumption</Text>
            <View style={styles.chartBadge}>
              <Text style={styles.chartBadgeText}>Last 7 days</Text>
            </View>
          </View>

          <BarChart
            data={{ labels: lastWeekLabels, datasets: [{ data: lastWeekKwh }] }}
            width={screenWidth - 55}
            height={200}
            chartConfig={chartConfig}
            style={styles.chart}
            withInnerLines={false}
            showValuesOnTopOfBars
          />
        </View>

        <Text style={styles.sectionTitle}>Last 6 Months</Text>

        <View style={styles.summaryRow}>
          <View style={styles.summaryCard}>
            <Text style={styles.summaryLabel}>Total kWh</Text>
            <View style={styles.valueRow}>
              <Text style={styles.summaryValue}>
                {last6MonthsKwh.reduce((a, b) => a + b, 0)}
              </Text>
              <Text style={styles.summaryUnit}>kWh</Text>
            </View>
          </View>

          <View style={styles.summaryCard}>
            <Text style={styles.summaryLabel}>Total Cost</Text>
            <View style={styles.valueRow}>
              <Text style={[styles.summaryValue, { color: '#0f766e' }]}>
                EGP {last6MonthsCost.reduce((a, b) => a + b, 0).toFixed(2)}
              </Text>
            </View>
          </View>
        </View>

        <View style={styles.chartCard}>
          <View style={styles.chartHeader}>
            <Text style={styles.chartTitle}>Usage vs Bill Comparison</Text>
            <View style={styles.chartBadge}>
              <Text style={styles.chartBadgeText}>6 Months</Text>
            </View>
          </View>

          <View style={styles.legendRow}>
            <View style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: '#0f766e' }]} />
              <Text style={styles.legendText}>kWh</Text>
            </View>

            <View style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: '#22c55e' }]} />
              <Text style={styles.legendText}>Cost (EGP)</Text>
            </View>
          </View>

          <LineChart
            data={{
              labels: last6MonthsLabels,
              datasets: [{ data: last6MonthsKwh.map((v) => v || 0) }],
            }}
            width={screenWidth - 48}
            height={200}
            chartConfig={{
              ...chartConfig,
              color: (opacity = 1) => `rgba(34, 197, 94, ${opacity})`,
            }}
            bezier
            style={styles.chartLine}
            withInnerLines
          />
        </View>

        <View style={styles.tableCard}>
          <Text style={styles.tableTitle}>Monthly Breakdown</Text>

          <View style={styles.tableHeader}>
            <Text style={[styles.tableHeaderText, { flex: 1 }]}>Month</Text>
            <Text style={[styles.tableHeaderText, { flex: 1, textAlign: 'center' }]}>
              kWh
            </Text>
            <Text style={[styles.tableHeaderText, { flex: 1, textAlign: 'center' }]}>
              Cost
            </Text>
          </View>

          {last6MonthsLabels.map((month, index) => (
            <View
              key={month}
              style={[
                styles.tableRow,
                index === last6MonthsLabels.length - 1 && styles.tableRowLast,
              ]}
            >
              <Text style={[styles.tableCell, { flex: 1 }]}>{month}</Text>
              <Text style={[styles.tableCell, { flex: 1, textAlign: 'center' }]}>
                {last6MonthsKwh[index]}
              </Text>
              <Text
                style={[
                  styles.tableCell,
                  { flex: 1, textAlign: 'center', color: '#0f766e' },
                ]}
              >
                EGP {last6MonthsCost[index].toFixed(2)}
              </Text>
            </View>
          ))}
        </View>

        <View style={{ height: 100 }} />
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f4f7fb' },

  pageTopRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 20,
    paddingTop: 16,
    paddingBottom: 12,
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
  pageTitle: {
    fontSize: 24,
    fontWeight: '800',
    color: '#0f172a',
  },
  topSpacer: {
    width: 42,
  },

  scroll: { flex: 1, padding: 20, paddingBottom: 110 },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: '#0f172a',
    marginBottom: 16,
    marginTop: 8,
  },
  summaryRow: { flexDirection: 'row', gap: 12, marginBottom: 16 },
  summaryCard: {
    flex: 1,
    backgroundColor: '#ffffff',
    borderRadius: 20,
    padding: 20,
    borderWidth: 1,
    borderColor: '#e2e8f0',
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  summaryLabel: { fontSize: 13, color: '#64748b', fontWeight: '600' },
  valueRow: { flexDirection: 'row', alignItems: 'baseline', marginTop: 8 },
  summaryValue: { fontSize: 28, fontWeight: '900', color: '#0f172a' },
  summaryUnit: {
    fontSize: 14,
    color: '#64748b',
    fontWeight: '600',
    marginLeft: 4,
  },
  statsGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 12, marginBottom: 20 },
  statItem: {
    width: '47%',
    backgroundColor: '#ffffff',
    borderRadius: 16,
    padding: 16,
    borderWidth: 1,
    borderColor: '#e2e8f0',
    shadowColor: '#0f172a',
    shadowOpacity: 0.04,
    shadowRadius: 8,
    shadowOffset: { width: 0, height: 2 },
    elevation: 1,
  },
  statLabel: { fontSize: 12, color: '#64748b', fontWeight: '600', marginBottom: 4 },
  statValue: { fontSize: 18, fontWeight: '800', color: '#0f172a' },
  chartCard: {
    backgroundColor: '#ffffff',
    borderWidth: 1,
    borderColor: '#e2e8f0',
    borderRadius: 24,
    padding: 20,
    marginBottom: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  chartHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  chartTitle: { fontSize: 17, fontWeight: '800', color: '#0f172a' },
  chartBadge: {
    backgroundColor: '#ecfdf5',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 999,
  },
  chartBadgeText: { fontSize: 11, fontWeight: '700', color: '#0f766e' },
  chart: { borderRadius: 12, marginLeft: -8 },
  chartLine: { borderRadius: 12, marginLeft: -8 },
  legendRow: { flexDirection: 'row', justifyContent: 'center', gap: 24, marginBottom: 12 },
  legendItem: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  legendDot: { width: 10, height: 10, borderRadius: 5 },
  legendText: { fontSize: 12, color: '#64748b', fontWeight: '600' },
  tableCard: {
    backgroundColor: '#ffffff',
    borderWidth: 1,
    borderColor: '#e2e8f0',
    borderRadius: 20,
    padding: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 4 },
    elevation: 2,
  },
  tableTitle: { fontSize: 17, fontWeight: '800', color: '#0f172a', marginBottom: 16 },
  tableHeader: {
    flexDirection: 'row',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#e2e8f0',
  },
  tableHeaderText: {
    fontSize: 11,
    color: '#64748b',
    textTransform: 'uppercase',
    letterSpacing: 0.8,
    fontWeight: '700',
  },
  tableRow: {
    flexDirection: 'row',
    paddingVertical: 14,
    borderBottomWidth: 1,
    borderBottomColor: '#f1f5f9',
  },
  tableRowLast: { borderBottomWidth: 0 },
  tableCell: { fontSize: 14, color: '#0f172a', fontWeight: '500' },
});