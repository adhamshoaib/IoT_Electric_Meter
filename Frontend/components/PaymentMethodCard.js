import React from 'react';
import { View, Text, TouchableOpacity, Image } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import styles from '../styles';

export default function PaymentMethodCard({
  icon,
  title,
  subtitle,
  logos = [],
  onPress,
})
{
  return (
    <TouchableOpacity
      style={styles.paymentMethodCard}
      activeOpacity={0.85}
      onPress={onPress}
    >
      <View style={styles.paymentMethodLeft}>
        <View style={styles.paymentMethodIconWrap}>
          <Ionicons name={icon} size={22} color="#0f766e" />
        </View>

        <View style={styles.paymentMethodTextWrap}>
          <Text style={styles.paymentMethodTitle}>{title}</Text>
          <Text style={styles.paymentMethodSubtitle}>{subtitle}</Text>
        </View>
      </View>

      <View style={styles.paymentLogoInlineRow}>
        {logos.map((logo, index) => (
          <React.Fragment key={index}>
            <Image
  source={logo}
  style={[
    styles.paymentLogoInlineImage,
    title === 'Mobile Wallet' && index === 0 && styles.vodafoneLogoImage,
    title === 'Fawry' && styles.fawryLogoImage,
  ]}
/>

            {index < logos.length - 1 && (
              <Text style={styles.paymentLogoComma}>,</Text>
            )}
          </React.Fragment>
        ))}
      </View>
    </TouchableOpacity>
  );
}