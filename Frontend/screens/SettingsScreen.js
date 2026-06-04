import React from 'react';
import {
  View,
  Text,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import styles from '../styles';
import SettingsRow from '../components/SettingsRow';



export default function SettingsScreen({ onBack, onOpenProfile, onOpenBilling, onLogout }) {
  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Settings</Text>

        <View style={styles.topSpacer} />
      </View>

      <View style={styles.sectionCard}>
        <SettingsRow
          icon="person-outline"
          title="Profile"
          subtitle="View account and meter details"
          onPress={onOpenProfile}
        />

        <SettingsRow
          icon="card-outline"
          title="Billing"
          subtitle="View payments and monthly estimate"
          onPress={onOpenBilling}
          lastItem
        />
      </View>

      <TouchableOpacity style={styles.logoutButton} onPress={onLogout}>
        <Text style={styles.logoutButtonText}>Logout</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}
