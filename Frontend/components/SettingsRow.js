import React from 'react';
import { View, Text, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import styles from '../styles';

export default function SettingsRow({ icon, title, subtitle, onPress, lastItem }) {
  return (
    <TouchableOpacity
      style={[styles.settingsRow, lastItem && styles.settingsRowLast]}
      onPress={onPress}
      activeOpacity={0.8}
    >
      <View style={styles.settingsRowLeft}>
        <View style={styles.settingsIconWrap}>
          <Ionicons name={icon} size={20} color="#0f766e" />
        </View>

        <View style={styles.settingsTextWrap}>
          <Text style={styles.settingsRowTitle}>{title}</Text>
          <Text style={styles.settingsRowSubtitle}>{subtitle}</Text>
        </View>
      </View>

      <Ionicons name="chevron-forward" size={18} color="#94a3b8" />
    </TouchableOpacity>
  );
}
