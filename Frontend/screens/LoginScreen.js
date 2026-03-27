import { useState } from 'react';
import {
  View, Text, TextInput, TouchableOpacity,
  StyleSheet, ActivityIndicator, KeyboardAvoidingView,
  Platform, ScrollView
} from 'react-native';

export default function LoginScreen({ navigation }) {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [showPass, setShowPass] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const handleLogin = () => {
    if (!email || !password) {
      setError('Please fill in all fields.');
      return;
    }
    setError('');
    setLoading(true);
    setTimeout(() => {
      setLoading(false);
      navigation.navigate('Dashboard');
    }, 1500);
  };

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
    >
      <ScrollView contentContainerStyle={styles.scroll} keyboardShouldPersistTaps="handled">

        {/* Brand */}
        <View style={styles.brandContainer}>
          <View style={styles.logoBox}>
            <Text style={styles.logoIcon}>⚡</Text>
          </View>
          <Text style={styles.brandText}>
            Han5ls <Text style={styles.brandAccent}>Inshallah</Text>
          </Text>
          <Text style={styles.brandSub}>Graduation Project · Sign in to continue</Text>
        </View>

        {/* Card */}
        <View style={styles.card}>

          {/* Top accent */}
          <View style={styles.topAccent} />

          {/* Email */}
          <Text style={styles.label}>EMAIL ADDRESS</Text>
          <TextInput
            style={styles.input}
            placeholder="ahmed@example.com"
            placeholderTextColor="#334155"
            value={email}
            onChangeText={setEmail}
            keyboardType="email-address"
            autoCapitalize="none"
          />

          {/* Password */}
          <Text style={styles.label}>PASSWORD</Text>
          <View style={styles.passRow}>
            <TextInput
              style={[styles.input, { flex: 1, marginBottom: 0 }]}
              placeholder="••••••••"
              placeholderTextColor="#334155"
              value={password}
              onChangeText={setPassword}
              secureTextEntry={!showPass}
            />
            <TouchableOpacity onPress={() => setShowPass(!showPass)} style={styles.showBtn}>
              <Text style={styles.showBtnText}>{showPass ? 'HIDE' : 'SHOW'}</Text>
            </TouchableOpacity>
          </View>

          {/* Error */}
          {error ? (
            <View style={styles.errorBox}>
              <Text style={styles.errorText}>{error}</Text>
            </View>
          ) : null}

          {/* Login Button */}
          <TouchableOpacity
            style={styles.loginBtn}
            onPress={handleLogin}
            disabled={loading}
            activeOpacity={0.85}
          >
            {loading
              ? <ActivityIndicator color="#080d14" />
              : <Text style={styles.loginBtnText}>Sign In</Text>
            }
          </TouchableOpacity>

          <Text style={styles.footerText}>
            Don't have an account?{' '}
            <Text style={styles.footerLink}>Contact admin</Text>
          </Text>

          {/* Bottom icons */}
          <View style={styles.iconsRow}>
            {[['🔒', 'Secure'], ['⚡', 'Real-time'], ['📊', 'Analytics']].map(([icon, label]) => (
              <View key={label} style={styles.iconItem}>
                <Text style={styles.iconEmoji}>{icon}</Text>
                <Text style={styles.iconLabel}>{label}</Text>
              </View>
            ))}
          </View>

        </View>
      </ScrollView>
    </KeyboardAvoidingView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#080d14',
  },
  scroll: {
    flexGrow: 1,
    justifyContent: 'center',
    padding: 24,
  },
  brandContainer: {
    alignItems: 'center',
    marginBottom: 32,
  },
  logoBox: {
    width: 60, height: 60, borderRadius: 16,
    backgroundColor: 'rgba(0,210,200,0.12)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.25)',
    alignItems: 'center', justifyContent: 'center',
    marginBottom: 14,
  },
  logoIcon: { fontSize: 28 },
  brandText: {
    fontSize: 26, fontWeight: '800', color: '#f8fafc',
  },
  brandAccent: { color: '#00d2c8' },
  brandSub: { fontSize: 12, color: '#334155', marginTop: 4 },
  card: {
    backgroundColor: 'rgba(15,22,35,0.95)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.15)',
    borderRadius: 20, padding: 28, overflow: 'hidden',
  },
  topAccent: {
    position: 'absolute', top: 0, left: 40, right: 40, height: 2,
    backgroundColor: 'rgba(0,210,200,0.4)',
    borderRadius: 999,
  },
  label: {
    fontSize: 11, color: '#64748b', fontWeight: '600',
    letterSpacing: 1, marginBottom: 6, marginTop: 16,
  },
  input: {
    backgroundColor: 'rgba(255,255,255,0.04)',
    borderWidth: 1, borderColor: 'rgba(0,210,200,0.2)',
    borderRadius: 10, padding: 14,
    color: '#e2e8f0', fontSize: 14, marginBottom: 4,
  },
  passRow: {
    flexDirection: 'row', alignItems: 'center', gap: 8, marginBottom: 4,
  },
  showBtn: {
    padding: 14,
    backgroundColor: 'rgba(0,210,200,0.08)',
    borderRadius: 10, borderWidth: 1,
    borderColor: 'rgba(0,210,200,0.2)',
  },
  showBtnText: { color: '#00d2c8', fontSize: 11, fontWeight: '700' },
  errorBox: {
    backgroundColor: 'rgba(248,113,113,0.1)',
    borderWidth: 1, borderColor: 'rgba(248,113,113,0.3)',
    borderRadius: 8, padding: 10, marginTop: 8,
  },
  errorText: { color: '#f87171', fontSize: 13 },
  loginBtn: {
    backgroundColor: '#00d2c8',
    borderRadius: 10, padding: 15,
    alignItems: 'center', marginTop: 20,
  },
  loginBtnText: {
    color: '#080d14', fontSize: 15, fontWeight: '800',
  },
  footerText: {
    textAlign: 'center', color: '#334155',
    fontSize: 12, marginTop: 16,
  },
  footerLink: { color: '#00d2c8', fontWeight: '600' },
  iconsRow: {
    flexDirection: 'row', justifyContent: 'center',
    gap: 32, marginTop: 24, paddingTop: 20,
    borderTopWidth: 1, borderTopColor: 'rgba(255,255,255,0.05)',
  },
  iconItem: { alignItems: 'center' },
  iconEmoji: { fontSize: 20 },
  iconLabel: { fontSize: 10, color: '#334155', marginTop: 4 },
});
