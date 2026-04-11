import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function MetricCard({ title, value, unit }) {
  return (
    <View style={styles.card}>
      <Text style={styles.cardTitle}>{title}</Text>
      <Text style={styles.cardValue}>
        {value} <Text style={styles.cardUnit}>{unit}</Text>
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#ffffff',
    padding: 18,
    borderRadius: 12,
    marginBottom: 14,
  },
  cardTitle: {
    fontSize: 16,
    color: '#666',
    marginBottom: 8,
  },
  cardValue: {
    fontSize: 28,
    fontWeight: 'bold',
  },
  cardUnit: {
    fontSize: 18,
    fontWeight: 'normal',
    color: '#444',
  },
});