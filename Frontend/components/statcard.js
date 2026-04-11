import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatCard({ title, value, subtitle, status }) {
  return (
    <View style={styles.card}>
      <View style={styles.topRow}>
        <Text style={styles.title}>{title}</Text>
        {status ? (
          <View
            style={[
              styles.statusBadge,
              status === 'online' ? styles.onlineBadge : styles.defaultBadge,
            ]}
          >
            <Text
              style={[
                styles.statusText,
                status === 'online' ? styles.onlineText : styles.defaultText,
              ]}
            >
              {status}
            </Text>
          </View>
        ) : null}
      </View>

      <Text style={styles.value}>{value}</Text>

      {subtitle ? <Text style={styles.subtitle}>{subtitle}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    backgroundColor: '#ffffff',
    borderRadius: 22,
    padding: 18,
    shadowColor: '#0f172a',
    shadowOpacity: 0.06,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  topRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  title: {
    fontSize: 13,
    color: '#64748b',
    fontWeight: '600',
  },
  value: {
    marginTop: 14,
    fontSize: 26,
    fontWeight: '800',
    color: '#0f172a',
  },
  subtitle: {
    marginTop: 8,
    fontSize: 12,
    color: '#94a3b8',
  },
  statusBadge: {
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 999,
  },
  onlineBadge: {
    backgroundColor: '#dcfce7',
  },
  defaultBadge: {
    backgroundColor: '#e2e8f0',
  },
  statusText: {
    fontSize: 11,
    fontWeight: '700',
  },
  onlineText: {
    color: '#15803d',
  },
  defaultText: {
    color: '#334155',
  },
});