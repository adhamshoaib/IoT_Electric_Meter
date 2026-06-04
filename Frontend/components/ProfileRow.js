import React from 'react';
import { View, Text } from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import styles from '../styles';

export default function ProfileRow({ icon, label, value, lastItem }) {
  return (
    <View style={[styles.profileInfoRow, lastItem && styles.profileInfoRowLast]}>
      <View style={styles.profileInfoIconBox}>
        <Ionicons name={icon} size={22} color="#0f766e" />
      </View>

      <View style={styles.profileInfoContent}>
        <Text style={styles.profileInfoLabel}>{label}</Text>
        <Text style={styles.profileInfoValue}>{value}</Text>
      </View>
    </View>
  );
}