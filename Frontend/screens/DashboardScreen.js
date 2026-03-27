import { useState } from 'react';
import {
  View, Text, ScrollView, TouchableOpacity,
  StyleSheet, Dimensions
} from 'react-native';
import { LineChart, BarChart } from 'react-native-chart-kit';

const screenWidth = Dimensions.get('window').width;

const chartConfig = {
  backgroundGradientFrom: '#0f1623',
  backgroundGradientTo: '#080d14',
  color: (opacity = 1) => `rgba(0, 210, 200, ${opacity})`,
  labelColor: (opacity = 1) => `rgba(71, 85, 105, ${opacity})`,
  strokeWidth: 2.5,
  propsForDots: { r: '4', strokeWidth: '0', stroke: '#00d2c8' },
  propsForBackgroundLines: { stroke: 'rgba(255,255,255,0.04)' },
};

const monthlyLabels = ['Aug', 'Sep', 'Oct', 'Nov', 'Dec', 'Jan', 'Feb', 'Mar'];
const monthlyKwh = [430, 370, 295, 315, 380, 340, 310, 315];
const weeklyLabels = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
const weeklyKwh = [12.4, 10.8, 13.1, 11.5, 14.2, 16.8, 15.3];

const KPICard = ({ label, value, unit, trend, icon, trendUp }) => (
  <View style={styles.kpiCard}>
    <View style={styles.kpiTopAccent} />
    <Text style={styles.kpiIcon}>{icon}</Text>
    <Text style={styles.kpiLabel}>{label}</Text>
    <Text style={styles.kpiValue}>
      {value}<Text style={styles.kpiUnit}> {unit}</Text>
    </Text>
    <View style={[styles.trendBadge, { backgroundColor: trendUp ? 'rgba(248,113,113,0.12)' : 'rgba(74,222,128,0.12)' }]}>
      <Text style={[styles.trendText, { color: trendUp ? '#f87171' : '#4ade80' }]}>
        {trendUp ? '▲' : '▼'} {trend}%
      </Text>
    </View>
  </View>
);

export default function DashboardScreen({ navigation }) {
  const [activeTab, setActiveTab] = useState('overview');

  return (
    <View style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <View>
          <Text style={styles.brandText}>Hangls <Text style={styles.brandAccent}>Inshallah</Text></Text>
          <Text style={styles.brandSub}>Electricity Dashboard</Text>
        </View>
        <TouchableOpacity onPress={() => navigation.navigate('Profile')} style={styles.avatar}>
          <Text style={styles.avatarText}>👤</Text>
        </TouchableOpacity>
      </View>

      {/* Tabs */}
      <View style={styles.tabsRow}>
        {['overview', 'bills', 'compare'].map(t => (
          <TouchableOpacity
            key={t}
            style={[styles.tabBtn, activeTab === t && styles.tabBtnActive]}
            onPress={() => {
              if (t === 'bills') navigation.navigate('Bills');
              else if (t === 'compare') navigation.navigate('Compare');
              else setActiveTab(t);
            }}
          >
            <Text style={[styles.tabText, activeTab === t && styles.tabTextActive]}>
              {t.charAt(0).toUpperCase() + t.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <ScrollView style={styles.scroll} showsVerticalScrollIndicator={false}>

        {/* Billing Hero */}
        <View style={styles.heroCard}>
          <View style={styles.heroTopAccent} />
          <Text style={styles.heroLabel}>CURRENT BILLING PERIOD · MAR 2026</Text>
          <Text style={styles.heroKwh}>315 <Text style={styles.heroUnit}>kWh</Text></Text>
          <Text style={styles.heroBill}>Estimated bill: <Text style={{ color: '#f8fafc', fontWeight: '700' }}>EGP 63.00</Text></Text>
          <View style={styles.heroStats}>
            {[['Daily Avg', '10.5 kWh'], ['Peak Hour', '8 PM'], ['Budget Used', '63%']].map(([l, v]) => (
              <View key={l} style={styles.heroStatItem}>
                <Text style={styles.heroStatLabel}>{l}</Text>
                <Text style={styles.heroStatValue}>{v}</Text>
              </View>
            ))}
          </View>
          <View style={styles.trendBadge}>
            <Text style={styles.trendText}>▼ 4.5% vs last month</Text>
          </View>
        </View>

        {/* KPI Cards */}
        <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.kpiScroll}>
          <KPICard label="This Month" value="315" unit="kWh" trend={4.5} icon="⚡" trendUp={false} />
          <KPICard label="Total Cost" value="EGP 63" unit="" trend={4.5} icon="💰" trendUp={false} />
          <KPICard label="CO₂ Emitted" value="126" unit="kg" trend={4.5} icon="🌿" trendUp={false} />
          <KPICard label="Peak Demand" value="2.8" unit="kW" trend={12} icon="📈" trendUp={true} />
        </ScrollView>

        {/* Line Chart */}
        <View style={styles.chartCard}>
          <View style={styles.chartTopAccent} />
          <Text style={styles.chartTitle}>Monthly Consumption</Text>
          <Text style={styles.chartSub}>Past 8 months trend</Text>
          <LineChart
            data={{ labels: monthlyLabels, datasets: [{ data: monthlyKwh }] }}
            width={screenWidth - 48}
            height={200}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
            withInnerLines={true}
            withOuterLines={false}
          />
        </View>

        {/* Bar Chart */}
        <View style={styles.chartCard}>
          <View style={styles.chartTopAccent} />
          <Text style={styles.chartTitle}>This Week</Text>
          <Text style={styles.chartSub}>Daily breakdown</Text>
          <BarChart
            data={{ labels: weeklyLabels, datasets: [{ data: weeklyKwh }] }}
            width={screenWidth - 48}
            height={200}
            chartConfig={chartConfig}
            style={styles.chart}
            withInnerLines={true}
            showValuesOnTopOfBars={false}
          />
        </View>

        {/* Saving Tips */}
        <View style={styles.tipsCard}>
          <View style={styles.chartTopAccent} />
          <Text style={styles.chartTitle}>💡 Saving Recommendations</Text>
          {[
            { tip: 'Shift laundry to off-peak hours (10 PM–6 AM)', save: '~18 kWh/mo' },
            { tip: 'Replace incandescent bulbs with LED', save: '~12 kWh/mo' },
            { tip: 'Reduce AC setpoint by 1°C during peak hours', save: '~25 kWh/mo' },
          ].map(({ tip, save }, i) => (
            <View key={i} style={[styles.tipRow, i < 2 && styles.tipBorder]}>
              <Text style={styles.tipText}>{tip}</Text>
              <View style={styles.saveBadge}>
                <Text style={styles.saveText}>{save}</Text>
              </View>
            </View>
          ))}
        </View>

        <View style={{ height: 32 }} />
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#080d14' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    padding: 20, paddingTop: 50,
    borderBottomWidth: 1, borderBottomColor: 'rgba(0,210,200,0.1)',
    backgroundColor: 'rgba(8,13,20,0.95)',
  },
  brandText: { fontSize: 18, fontWeight: '800', color: '#f8fafc' },
  brandAccent: { color: '#00d2c8' },
  brandSub: { fontSize: 11, color: '#334155', marginTop: 2 },
  avatar: {
    width: 38, height: 38, borderRadius: 19,
    backgroundColor: 'rgba(30,33,48,0.9)',
    borderWidth: 2, borderColor: 'rgba(0,210,200,0.3)',
    alignItems: 'center', justifyContent: 'center',
  },
  avatarText: { fontSize: 18 },
  tabsRow: {
    flexDirection: 'row', gap: 8, padding: 12, paddingHorizontal: 20,
    backgroundColor: 'rgba(8,13,20,0.95)',
  },
  tabBtn: {
    paddingHorizontal: 16, paddingVertical: 8, borderRadius: 8,
    borderWidth: 1, borderColor: 'rgba(255,255,255,0.08)',
  },
  tabBtnActive: {
    backgroundColor: 'rgba(0,210,200,0.1)',
    borderColor: 'rgba(0,210,200,0.4)',
  },
  tabText: { fontSize: 13, color: '#64748b', fontWeight: '500', textTransform: 'capitalize' },
  tabTextActive: { color: '#00d2c8' },
  scroll: { flex: 1, padding: 16 },
  heroCard: {
    backgroundColor: 'rgba(15,22,35,0.95)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.15)',
    borderRadius: 16, padding: 24, marginBottom: 16, overflow: 'hidden',
  },
  heroTopAccent: {
    position: 'absolute', top: 0, left: 40, right: 40, height: 2,
    backgroundColor: 'rgba(0,210,200,0.4)', borderRadius: 999,
  },
  heroLabel: { fontSize: 11, color: '#64748b', letterSpacing: 1, marginBottom: 8 },
  heroKwh: { fontSize: 48, fontWeight: '800', color: '#00d2c8', lineHeight: 52 },
  heroUnit: { fontSize: 18, color: '#94a3b8', fontWeight: '400' },
  heroBill: { fontSize: 13, color: '#94a3b8', marginTop: 4, marginBottom: 16 },
  heroStats: { flexDirection: 'row', gap: 24, marginBottom: 16 },
  heroStatLabel: { fontSize: 10, color: '#475569', textTransform: 'uppercase', letterSpacing: 0.8 },
  heroStatValue: { fontSize: 16, fontWeight: '700', color: '#f8fafc', marginTop: 2 },
  trendBadge: {
    alignSelf: 'flex-start', backgroundColor: 'rgba(74,222,128,0.12)',
    borderRadius: 999, paddingHorizontal: 12, paddingVertical: 4,
    borderWidth: 1, borderColor: 'rgba(74,222,128,0.25)',
  },
  trendText: { fontSize: 12, color: '#4ade80', fontWeight: '600' },
  kpiScroll: { marginBottom: 16 },
  kpiCard: {
    backgroundColor: 'rgba(15,22,35,0.95)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.12)',
    borderRadius: 14, padding: 18, marginRight: 12,
    width: 150, overflow: 'hidden',
  },
  kpiTopAccent: {
    position: 'absolute', top: 0, left: 20, right: 20, height: 2,
    backgroundColor: 'rgba(0,210,200,0.35)', borderRadius: 999,
  },
  kpiIcon: { fontSize: 22, marginBottom: 8 },
  kpiLabel: { fontSize: 10, color: '#64748b', textTransform: 'uppercase', letterSpacing: 0.8, marginBottom: 4 },
  kpiValue: { fontSize: 22, fontWeight: '800', color: '#f8fafc', marginBottom: 8 },
  kpiUnit: { fontSize: 12, color: '#94a3b8', fontWeight: '400' },
  chartCard: {
    backgroundColor: 'rgba(15,22,35,0.95)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.12)',
    borderRadius: 16, padding: 20, marginBottom: 16, overflow: 'hidden',
  },
  chartTopAccent: {
    position: 'absolute', top: 0, left: 40, right: 40, height: 2,
    backgroundColor: 'rgba(0,210,200,0.35)', borderRadius: 999,
  },
  chartTitle: { fontSize: 15, fontWeight: '700', color: '#f8fafc', marginBottom: 4 },
  chartSub: { fontSize: 12, color: '#475569', marginBottom: 16 },
  chart: { borderRadius: 12, marginLeft: -16 },
  tipsCard: {
    backgroundColor: 'rgba(15,22,35,0.95)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.12)',
    borderRadius: 16, padding: 20, marginBottom: 16, overflow: 'hidden',
  },
  tipRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', paddingVertical: 14, gap: 12,
  },
  tipBorder: { borderBottomWidth: 1, borderBottomColor: 'rgba(255,255,255,0.05)' },
  tipText: { fontSize: 13, color: '#94a3b8', flex: 1 },
  saveBadge: {
    backgroundColor: 'rgba(74,222,128,0.1)', borderRadius: 999,
    paddingHorizontal: 10, paddingVertical: 3,
    borderWidth: 1, borderColor: 'rgba(74,222,128,0.2)',
  },
  saveText: { fontSize: 11, color: '#4ade80', fontWeight: '600' },
});
