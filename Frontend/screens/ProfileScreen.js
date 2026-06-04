import React from 'react';
import {
  View,
  Text,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import ProfileRow from '../components/ProfileRow';
import styles from '../styles';
export default 
function ProfileScreen({ user, onBack }) {
  return (
    <ScrollView
      contentContainerStyle={styles.scrollContent}
      showsVerticalScrollIndicator={false}
    >
      <View style={styles.pageTopRow}>
        <TouchableOpacity style={styles.backButton} onPress={onBack}>
          <Ionicons name="chevron-back" size={22} color="#0f172a" />
        </TouchableOpacity>

        <Text style={styles.pageTitle}>Profile</Text>

        <View style={styles.topSpacer} />
      </View>

      <View style={styles.profileHero}>
        <View style={styles.profileAvatar}>
          <Text style={styles.profileAvatarText}>{user.name.charAt(0)}</Text>
        </View>

        <Text style={styles.profileName}>{user.name}</Text>
        <Text style={styles.profileEmail}>{user.email}</Text>
      </View>

      <View style={styles.sectionCard}>
        <Text style={styles.sectionTitle}>Account Info</Text>
<ProfileRow
  icon="speedometer-outline"
  label="Meter ID"
  value={user?.meterId ?? 'Not added'}
/>

<ProfileRow
  icon="card-outline"
  label="Account Number"
  value={user?.accountNumber ?? 'Not added'}
  lastItem
/>


      </View>
    </ScrollView>
  );
}