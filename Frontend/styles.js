import { StyleSheet } from 'react-native';



const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f4f7fb',
  },

  scrollContent: {
    padding: 20,
    paddingBottom: 110,
  },

  loginScreen: {
    flex: 1,
    justifyContent: 'center',
    padding: 24,
    overflow: 'hidden',
  },
  loginGlowTop: {
    position: 'absolute',
    top: -60,
    right: -40,
    width: 180,
    height: 180,
    borderRadius: 90,
    backgroundColor: '#d1fae5',
    opacity: 0.8,
  },
  loginGlowBottom: {
    position: 'absolute',
    bottom: -50,
    left: -40,
    width: 160,
    height: 160,
    borderRadius: 80,
    backgroundColor: '#dbeafe',
    opacity: 0.8,
  },
  logoCircle: {
    width: 82,
    height: 82,
    borderRadius: 41,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
    alignSelf: 'center',
    marginBottom: 18,
    shadowColor: '#0f766e',
    shadowOpacity: 0.22,
    shadowRadius: 16,
    shadowOffset: { width: 0, height: 8 },
    elevation: 5,
  },
  logoIcon: {
    fontSize: 34,
    color: '#ffffff',
  },
  appTitle: {
    fontSize: 32,
    fontWeight: '800',
    color: '#0f172a',
    textAlign: 'center',
  },
  appSubtitle: {
    fontSize: 14,
    color: '#64748b',
    textAlign: 'center',
    marginTop: 8,
    marginBottom: 28,
    lineHeight: 21,
  },
  loginCard: {
    backgroundColor: '#ffffff',
    borderRadius: 26,
    padding: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.06,
    shadowRadius: 18,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  inputLabel: {
    fontSize: 14,
    color: '#334155',
    fontWeight: '600',
    marginBottom: 8,
    marginTop: 12,
  },
  authTitle: {
  fontSize: 22,
  fontWeight: '800',
  color: '#0f172a',
  marginBottom: 6,
},

authSubtitle: {
  fontSize: 13,
  color: '#64748b',
  lineHeight: 19,
  marginBottom: 10,
},

authError: {
  color: '#dc2626',
  backgroundColor: '#fef2f2',
  borderWidth: 1,
  borderColor: '#fecaca',
  padding: 12,
  borderRadius: 14,
  fontSize: 13,
  marginTop: 14,
},

disabledButton: {
  opacity: 0.65,
},

signUpText: {
  textAlign: 'center',
  marginTop: 18,
  fontSize: 14,
  fontWeight: '700',
  color: '#0f766e',
},
  input: {
    backgroundColor: '#f8fafc',
    borderWidth: 1,
    borderColor: '#e2e8f0',
    borderRadius: 16,
    paddingHorizontal: 16,
    paddingVertical: 14,
    fontSize: 15,
    color: '#0f172a',
  },
  loginButton: {
    backgroundColor: '#0f766e',
    borderRadius: 16,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 24,
    shadowColor: '#0f766e',
    shadowOpacity: 0.2,
    shadowRadius: 12,
    shadowOffset: { width: 0, height: 8 },
    elevation: 4,
  },
  loginButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '700',
  },

  headerRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  welcomeText: {
    fontSize: 28,
    fontWeight: '800',
    color: '#0f172a',
    maxWidth: '80%',
  },
  headerSubtitle: {
    fontSize: 14,
    color: '#64748b',
    marginTop: 6,
  },
  headerAvatar: {
    width: 46,
    height: 46,
    borderRadius: 23,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
  },
  headerAvatarText: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '800',
  },

  heroCard: {
    backgroundColor: '#0f766e',
    borderRadius: 30,
    padding: 24,
    marginTop: 22,
    marginBottom: 20,
    overflow: 'hidden',
    shadowColor: '#0f766e',
    shadowOpacity: 0.18,
    shadowRadius: 20,
    shadowOffset: { width: 0, height: 10 },
    elevation: 6,
  },
  heroGlowOne: {
    position: 'absolute',
    width: 170,
    height: 170,
    borderRadius: 85,
    backgroundColor: '#14b8a6',
    top: -50,
    right: -30,
    opacity: 0.18,
  },
  heroGlowTwo: {
    position: 'absolute',
    width: 110,
    height: 110,
    borderRadius: 55,
    backgroundColor: '#99f6e4',
    bottom: -25,
    left: -15,
    opacity: 0.12,
  },
  heroBadge: {
    alignSelf: 'flex-start',
    backgroundColor: 'rgba(255,255,255,0.16)',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 999,
  },
  heroBadgeText: {
    color: '#ffffff',
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 0.8,
  },
  heroLabel: {
    color: '#d1fae5',
    fontSize: 16,
    fontWeight: '600',
    marginTop: 18,
    marginBottom: 12,
  },
  heroValueRow: {
    flexDirection: 'row',
    alignItems: 'flex-end',
  },
  heroValue: {
    color: '#ffffff',
    fontSize: 58,
    fontWeight: '900',
    lineHeight: 62,
  },
  heroUnit: {
    color: '#d1fae5',
    fontSize: 21,
    fontWeight: '700',
    marginLeft: 8,
    marginBottom: 8,
  },
  heroNote: {
    color: '#ccfbf1',
    fontSize: 13,
    marginTop: 14,
    lineHeight: 20,
    maxWidth: '90%',
  },

  balanceSection: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    paddingBottom: 18,
  },
  balanceSectionLabel: {
    fontSize: 14,
    color: '#64748b',
    marginBottom: 8,
  },
  balanceSectionValue: {
    fontSize: 34,
    fontWeight: '800',
    color: '#0f766e',
  },
  balanceMiniPill: {
    backgroundColor: '#e6fffa',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 999,
  },
  balanceMiniPillText: {
    fontSize: 12,
    fontWeight: '700',
    color: '#0f766e',
  },

  billingPreviewCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    marginBottom: 20,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  billingPreviewHeader: {
    marginBottom: 14,
  },
  billingPreviewTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
  },
  billingPreviewSubtitle: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 4,
  },
  billingPreviewRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  billingPreviewItem: {
    width: '48%',
    backgroundColor: '#f8fafc',
    borderRadius: 18,
    padding: 14,
  },
  billingPreviewItemLabel: {
    fontSize: 12,
    color: '#64748b',
    marginBottom: 8,
  },
  billingPreviewItemValue: {
    fontSize: 20,
    fontWeight: '800',
    color: '#0f172a',
  },
  billingPreviewItemDate: {
    fontSize: 12,
    color: '#94a3b8',
    marginTop: 6,
  },

  divider: {
    height: 1,
    backgroundColor: '#e2e8f0',
    marginBottom: 20,
  },

  statusSection: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingBottom: 22,
  },
  statusIconArea: {
    width: 72,
    alignItems: 'center',
    marginRight: 14,
  },
  signalDotOuter: {
    width: 30,
    height: 30,
    borderRadius: 15,
    backgroundColor: '#dcfce7',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 10,
  },
  signalDotInner: {
    width: 12,
    height: 12,
    borderRadius: 6,
    backgroundColor: '#22c55e',
  },
  barsWrap: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    height: 20,
  },
  bar: {
    width: 5,
    backgroundColor: '#22c55e',
    borderRadius: 3,
    marginHorizontal: 2,
  },
  barOne: {
    height: 6,
  },
  barTwo: {
    height: 10,
  },
  barThree: {
    height: 14,
  },
  barFour: {
    height: 18,
  },
  statusTextWrap: {
    flex: 1,
  },
  statusEyebrow: {
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 0.8,
    color: '#94a3b8',
    marginBottom: 6,
  },
  statusTitle: {
    fontSize: 22,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 4,
  },
  statusSubtitle: {
    fontSize: 14,
    color: '#64748b',
    lineHeight: 20,
  },
  syncPill: {
    backgroundColor: '#ecfdf5',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 999,
    marginLeft: 10,
  },
  syncPillText: {
    color: '#0f766e',
    fontSize: 12,
    fontWeight: '700',
  },

  warningBanner: {
    backgroundColor: '#fff7ed',
    borderRadius: 22,
    padding: 16,
    borderWidth: 1,
    borderColor: '#fed7aa',
    flexDirection: 'row',
    alignItems: 'flex-start',
  },
  warningIconCircle: {
    width: 34,
    height: 34,
    borderRadius: 17,
    backgroundColor: '#fb923c',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  warningIcon: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '800',
  },
  warningContent: {
    flex: 1,
  },
  warningTitle: {
    fontSize: 15,
    fontWeight: '800',
    color: '#c2410c',
    marginBottom: 6,
  },
  warningText: {
    fontSize: 14,
    lineHeight: 20,
    color: '#9a3412',
  },

  pageTopRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 20,
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

  sectionCard: {
    backgroundColor: '#ffffff',
    borderRadius: 24,
    padding: 18,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  sectionCardSpacing: {
    marginTop: 16,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 8,
  },

  settingsRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  settingsRowLast: {
    borderBottomWidth: 0,
  },
  settingsRowLeft: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    marginRight: 12,
  },
  settingsIconWrap: {
    width: 42,
    height: 42,
    borderRadius: 21,
    backgroundColor: '#ecfdf5',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  settingsTextWrap: {
    flex: 1,
  },
  settingsRowTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: '#0f172a',
    marginBottom: 4,
  },
  settingsRowSubtitle: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 18,
  },

  profileHero: {
    backgroundColor: '#ffffff',
    borderRadius: 28,
    paddingVertical: 28,
    paddingHorizontal: 20,
    alignItems: 'center',
    marginBottom: 16,
    shadowColor: '#0f172a',
    shadowOpacity: 0.05,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 8 },
    elevation: 3,
  },
  profileAvatar: {
    width: 92,
    height: 92,
    borderRadius: 46,
    backgroundColor: '#0f766e',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 14,
  },
  profileAvatarText: {
    color: '#ffffff',
    fontSize: 36,
    fontWeight: '800',
  },
  profileName: {
    fontSize: 28,
    fontWeight: '800',
    color: '#0f172a',
  },
  profileEmail: {
    fontSize: 14,
    color: '#64748b',
    marginTop: 6,
  },
  profileRow: {
    paddingVertical: 18,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  profileRowLast: {
    borderBottomWidth: 0,
  },
  profileRowLabel: {
    fontSize: 12,
    color: '#94a3b8',
    textTransform: 'uppercase',
    fontWeight: '700',
    marginBottom: 6,
  },
  profileRowValue: {
    fontSize: 16,
    color: '#0f172a',
    fontWeight: '600',
  },

  billingSummaryCard: {
    backgroundColor: '#0f766e',
    borderRadius: 28,
    padding: 22,
    marginBottom: 16,
    shadowColor: '#0f766e',
    shadowOpacity: 0.18,
    shadowRadius: 20,
    shadowOffset: { width: 0, height: 10 },
    elevation: 6,
  },
  billingEyebrow: {
    color: '#ccfbf1',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 0.8,
    marginBottom: 10,
  },
  billingMainValue: {
    color: '#ffffff',
    fontSize: 42,
    fontWeight: '900',
  },
  billingSummaryText: {
    color: '#d1fae5',
    fontSize: 14,
    lineHeight: 20,
    marginTop: 8,
    marginBottom: 18,
  },
  billingStatsRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  billingStatItem: {
    width: '48%',
    backgroundColor: 'rgba(255,255,255,0.14)',
    borderRadius: 18,
    padding: 14,
  },
  billingStatLabel: {
    color: '#ccfbf1',
    fontSize: 12,
    marginBottom: 6,
  },
  billingStatValue: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '800',
  },
  billingStatSubtext: {
    color: '#ccfbf1',
    fontSize: 12,
    marginTop: 6,
  },
  billingSectionHint: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 19,
    marginBottom: 12,
  },
  paymentMethodCard: {
    backgroundColor: '#f8fafc',
    borderRadius: 18,
    padding: 14,
    marginTop: 12,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  paymentMethodLeft: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    marginRight: 12,
  },
  paymentMethodIconWrap: {
    width: 46,
    height: 46,
    borderRadius: 23,
    backgroundColor: '#ecfdf5',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  paymentMethodTextWrap: {
    flex: 1,
  },
  paymentMethodTitle: {
    fontSize: 16,
    fontWeight: '800',
    color: '#0f172a',
    marginBottom: 4,
  },
  paymentMethodSubtitle: {
    fontSize: 13,
    color: '#64748b',
    lineHeight: 18,
  },
  paymentMethodTag: {
    backgroundColor: '#dcfce7',
    borderRadius: 999,
    paddingHorizontal: 10,
    paddingVertical: 6,
  },
  paymentMethodTagText: {
    fontSize: 11,
    fontWeight: '800',
    color: '#166534',
  },
  demoNote: {
    marginTop: 14,
    backgroundColor: '#ecfdf5',
    borderRadius: 16,
    padding: 12,
  },
  demoNoteText: {
    fontSize: 12,
    lineHeight: 18,
    color: '#0f766e',
    fontWeight: '600',
  },
  paymentRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#eef2f7',
  },
  paymentRowLast: {
    borderBottomWidth: 0,
  },
  paymentDate: {
    fontSize: 15,
    fontWeight: '700',
    color: '#0f172a',
  },
  paymentMethod: {
    fontSize: 13,
    color: '#64748b',
    marginTop: 4,
  },
  paymentAmount: {
    fontSize: 16,
    fontWeight: '800',
    color: '#0f766e',
  },

  logoutButton: {
    backgroundColor: '#ef4444',
    borderRadius: 16,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 20,
  },
  logoutButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '800',
  },

  tabBar: {
    position: 'absolute',
    left: 16,
    right: 16,
    bottom: 16,
    flexDirection: 'row',
    backgroundColor: '#ffffff',
    borderRadius: 22,
    padding: 8,
    shadowColor: '#0f172a',
    shadowOpacity: 0.08,
    shadowRadius: 14,
    shadowOffset: { width: 0, height: 6 },
    elevation: 5,
  },
  tabButton: {
    flex: 1,
    borderRadius: 14,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
  },
  activeTabButton: {
    backgroundColor: '#ecfdf5',
  },
  tabText: {
    fontSize: 14,
    fontWeight: '700',
    color: '#64748b',
  },
  activeTabText: {
    color: '#0f766e',
  },
  
heroMetaText: {
  color: '#ccfbf1',
  fontSize: 13,
  marginTop: 10,
  fontWeight: '600',
},
meterInputRow: {
  flexDirection: 'row',
  alignItems: 'center',
},

meterPrefixBox: {
  height: 50,
  paddingHorizontal: 16,
  borderTopLeftRadius: 16,
  borderBottomLeftRadius: 16,
  backgroundColor: '#ecfdf5',
  borderWidth: 1,
  borderColor: '#ccfbf1',
  alignItems: 'center',
  justifyContent: 'center',
},

meterPrefixText: {
  color: '#0f766e',
  fontSize: 15,
  fontWeight: '800',
},

meterNumberInput: {
  flex: 1,
  height: 50,
  backgroundColor: '#f8fafc',
  borderWidth: 1,
  borderLeftWidth: 0,
  borderColor: '#e2e8f0',
  borderTopRightRadius: 16,
  borderBottomRightRadius: 16,
  paddingHorizontal: 16,
  fontSize: 15,
  color: '#0f172a',
},

meterPreviewText: {
  marginTop: 8,
  fontSize: 12,
  color: '#64748b',
  fontWeight: '600',
},
paymentPageContent: {
  padding: 24,
  paddingBottom: 120,
},

paymentBackButton: {
  marginBottom: 18,
},

paymentBackText: {
  fontSize: 16,
  fontWeight: '800',
  color: '#0f766e',
},

cardVisual: {
  borderRadius: 28,
  padding: 24,
  minHeight: 190,
  backgroundColor: '#0f766e',
  justifyContent: 'space-between',
  marginBottom: 22,
},

cardVisualLabel: {
  color: '#ccfbf1',
  fontSize: 15,
  fontWeight: '700',
},

cardVisualNumber: {
  color: '#ffffff',
  fontSize: 24,
  fontWeight: '900',
  letterSpacing: 2,
  marginTop: 26,
},

cardVisualBottom: {
  flexDirection: 'row',
  justifyContent: 'space-between',
  marginTop: 26,
},

cardVisualSmallLabel: {
  color: '#99f6e4',
  fontSize: 11,
  fontWeight: '700',
  textTransform: 'uppercase',
},

cardVisualValue: {
  color: '#ffffff',
  fontSize: 14,
  fontWeight: '800',
  marginTop: 4,
},

paymentFormCard: {
  backgroundColor: '#ffffff',
  borderRadius: 28,
  padding: 22,
  shadowColor: '#000',
  shadowOpacity: 0.08,
  shadowRadius: 18,
  elevation: 5,
},

paymentPageTitle: {
  fontSize: 24,
  fontWeight: '900',
  color: '#0f172a',
},

paymentPageSubtitle: {
  marginTop: 6,
  marginBottom: 16,
  fontSize: 14,
  lineHeight: 22,
  color: '#64748b',
},

cardInputRow: {
  flexDirection: 'row',
  gap: 12,
},

payNowButton: {
  marginTop: 22,
  backgroundColor: '#0f766e',
  paddingVertical: 16,
  borderRadius: 18,
  alignItems: 'center',
},

payNowButtonText: {
  color: '#ffffff',
  fontSize: 16,
  fontWeight: '900',
},

paymentStatusMessage: {
  marginTop: 14,
  fontSize: 14,
  fontWeight: '700',
  color: '#dc2626',
},

paymentStatusSuccess: {
  color: '#16a34a',
},
walletHeroCard: {
  backgroundColor: '#ecfdf5',
  borderRadius: 28,
  padding: 24,
  marginBottom: 22,
  borderWidth: 1,
  borderColor: '#bbf7d0',
},

walletHeroIcon: {
  width: 62,
  height: 62,
  borderRadius: 22,
  backgroundColor: '#d1fae5',
  alignItems: 'center',
  justifyContent: 'center',
  marginBottom: 18,
},

walletHeroTitle: {
  fontSize: 26,
  fontWeight: '900',
  color: '#064e3b',
},

walletHeroSubtitle: {
  marginTop: 8,
  fontSize: 15,
  lineHeight: 23,
  color: '#047857',
},

walletHintText: {
  marginTop: 8,
  fontSize: 12,
  color: '#64748b',
  fontWeight: '600',
},
savedCardBox: {
  marginTop: 16,
  marginBottom: 10,
  padding: 16,
  borderRadius: 18,
  backgroundColor: '#f0fdfa',
  borderWidth: 1,
  borderColor: '#99f6e4',
  flexDirection: 'row',
  alignItems: 'center',
  justifyContent: 'space-between',
},

savedCardTitle: {
  fontSize: 15,
  fontWeight: '900',
  color: '#0f172a',
},

savedCardSubtitle: {
  marginTop: 4,
  fontSize: 12,
  fontWeight: '600',
  color: '#64748b',
},

changeCardText: {
  fontSize: 13,
  fontWeight: '900',
  color: '#0f766e',
},

rememberCardRow: {
  marginTop: 14,
  flexDirection: 'row',
  alignItems: 'center',
},

rememberCheckbox: {
  width: 22,
  height: 22,
  borderRadius: 7,
  borderWidth: 1,
  borderColor: '#94a3b8',
  alignItems: 'center',
  justifyContent: 'center',
  marginRight: 10,
},

rememberCheckboxActive: {
  backgroundColor: '#0f766e',
  borderColor: '#0f766e',
},

rememberCardText: {
  fontSize: 13,
  fontWeight: '700',
  color: '#334155',
},

removeSavedCardButton: {
  marginTop: 12,
  alignItems: 'center',
},

removeSavedCardText: {
  fontSize: 13,
  fontWeight: '800',
  color: '#dc2626',
},
paymentLogoRow: {
  alignItems: 'flex-end',
  justifyContent: 'center',
  gap: 6,
},
paymentLogoInlineRow: {
  flexDirection: 'row',
  alignItems: 'center',
  justifyContent: 'flex-end',
  flexShrink: 0,
  maxWidth: 145,
},

paymentLogoInlineImage: {
  width: 34,
  height: 20,
  resizeMode: 'contain',
},

paymentLogoInlineImageLarge: {
  width: 76,
  height: 30,
  resizeMode: 'contain',
},

paymentLogoComma: {
  marginHorizontal: 4,
  fontSize: 16,
  fontWeight: '800',
  color: '#64748b',
},
vodafoneLogoImage: {
  width: 75,
  height: 36,
  resizeMode: 'contain',
},

fawryLogoImage: {
  width: 100,
  height: 42,
  resizeMode: 'contain',
},
fawryHeroCard: {
  backgroundColor: '#eff6ff',
  borderRadius: 28,
  padding: 24,
  marginBottom: 22,
  borderWidth: 1,
  borderColor: '#bfdbfe',
},

fawryHeroTop: {
  flexDirection: 'row',
  alignItems: 'center',
  justifyContent: 'space-between',
  marginBottom: 18,
},

fawryHeroIcon: {
  width: 62,
  height: 62,
  borderRadius: 22,
  backgroundColor: '#dbeafe',
  alignItems: 'center',
  justifyContent: 'center',
},

fawryHeroLogo: {
  width: 110,
  height: 45,
  resizeMode: 'contain',
},

fawryHeroTitle: {
  fontSize: 26,
  fontWeight: '900',
  color: '#1e3a8a',
},

fawryHeroSubtitle: {
  marginTop: 8,
  fontSize: 15,
  lineHeight: 23,
  color: '#2563eb',
},

fawryReferenceBox: {
  marginTop: 20,
  padding: 18,
  borderRadius: 22,
  backgroundColor: '#f8fafc',
  borderWidth: 1,
  borderColor: '#e2e8f0',
},

fawryReferenceLabel: {
  fontSize: 13,
  fontWeight: '800',
  color: '#64748b',
  textAlign: 'center',
},

fawryReferenceCode: {
  marginTop: 8,
  marginBottom: 18,
  fontSize: 32,
  fontWeight: '900',
  color: '#0f172a',
  letterSpacing: 2,
  textAlign: 'center',
},

fawryInfoRow: {
  flexDirection: 'row',
  justifyContent: 'space-between',
  alignItems: 'center',
  paddingVertical: 9,
  borderBottomWidth: 1,
  borderBottomColor: '#e2e8f0',
},

fawryInfoLabel: {
  fontSize: 13,
  fontWeight: '700',
  color: '#64748b',
},

fawryInfoValue: {
  fontSize: 13,
  fontWeight: '900',
  color: '#0f172a',
  maxWidth: 180,
  textAlign: 'right',
},

fawryStepsBox: {
  marginTop: 18,
  marginBottom: 4,
  padding: 14,
  borderRadius: 18,
  backgroundColor: '#eff6ff',
},

fawryStepsTitle: {
  fontSize: 14,
  fontWeight: '900',
  color: '#1e3a8a',
  marginBottom: 8,
},

fawryStepText: {
  fontSize: 13,
  lineHeight: 21,
  fontWeight: '600',
  color: '#334155',
},
recentPaymentsHeader: {
  flexDirection: 'row',
  justifyContent: 'space-between',
  alignItems: 'flex-start',
  marginBottom: 14,
},

recentPaymentsSubtitle: {
  marginTop: 4,
  fontSize: 13,
  color: '#64748b',
  fontWeight: '600',
},

clearPaymentsText: {
  fontSize: 13,
  fontWeight: '900',
  color: '#dc2626',
},

emptyPaymentsBox: {
  paddingVertical: 22,
  alignItems: 'center',
  borderRadius: 18,
  backgroundColor: '#f8fafc',
},

emptyPaymentsText: {
  fontSize: 14,
  fontWeight: '700',
  color: '#64748b',
},

paymentStatusText: {
  marginTop: 4,
  fontSize: 12,
  fontWeight: '800',
  color: '#16a34a',
},

paymentRightSide: {
  alignItems: 'flex-end',
  justifyContent: 'center',
  marginLeft: 12,
},

deletePaymentButton: {
  marginTop: 8,
  paddingHorizontal: 12,
  paddingVertical: 6,
  borderRadius: 999,
  backgroundColor: '#fee2e2',
},

deletePaymentText: {
  fontSize: 12,
  fontWeight: '900',
  color: '#dc2626',
},
balanceAlertCard: {
  marginTop: 24,
  padding: 18,
  borderRadius: 24,
  backgroundColor: '#fffbeb',
  borderWidth: 1,
  borderColor: '#fde68a',
  flexDirection: 'row',
  alignItems: 'center',
  shadowColor: '#000',
  shadowOpacity: 0.06,
  shadowRadius: 12,
  elevation: 3,
},

balanceAlertCardDanger: {
  backgroundColor: '#fef2f2',
  borderColor: '#fecaca',
},

balanceAlertIconWrap: {
  width: 48,
  height: 48,
  borderRadius: 16,
  backgroundColor: '#f59e0b',
  alignItems: 'center',
  justifyContent: 'center',
  marginRight: 14,
},

balanceAlertIconWrapDanger: {
  backgroundColor: '#ef4444',
},

balanceAlertTextWrap: {
  flex: 1,
},

balanceAlertTitle: {
  fontSize: 16,
  fontWeight: '900',
  color: '#92400e',
},

balanceAlertTitleDanger: {
  color: '#991b1b',
},

balanceAlertSubtitle: {
  marginTop: 5,
  fontSize: 13,
  lineHeight: 20,
  fontWeight: '600',
  color: '#475569',
},
statusTextWrap: {
  flex: 1,
  marginLeft: 18,
  marginRight: 8,
},

statusEyebrow: {
  fontSize: 12,
  fontWeight: '900',
  color: '#94a3b8',
  letterSpacing: 1.4,
  marginBottom: 6,
},

meterStatusTitle: {
  fontSize: 20,
  fontWeight: '900',
  lineHeight: 24,
},

meterStatusSubtitle: {
  marginTop: 4,
  fontSize: 13,
  lineHeight: 18,
  fontWeight: '600',
  color: '#64748b',
},
});
export default styles;
