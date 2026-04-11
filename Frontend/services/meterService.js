export function getMeterData() {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        voltage: 231,
        current: 5.8,
        power: 1340,
        energy: 16.9,
        status: 'Online',
        lastUpdate: '06 Apr 2026, 6:55 PM',
      });
    }, 2000);
  });
}