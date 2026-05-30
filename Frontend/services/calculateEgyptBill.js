export function calculateEgyptBill(kwh) {
  const consumption = Number(kwh || 0);

  let energyCost = 0;
  let serviceFee = 0;

  if (consumption <= 0) {
    return {
      energyCost: 0,
      serviceFee: 0,
      totalCost: 0,
    };
  }

  if (consumption <= 50) {
    energyCost = consumption * 0.68;
    serviceFee = 1;
  } else if (consumption <= 100) {
    energyCost = 50 * 0.68 + (consumption - 50) * 0.78;
    serviceFee = 2;
  } else if (consumption <= 200) {
    energyCost = consumption * 0.95;
    serviceFee = 6;
  } else if (consumption <= 350) {
    energyCost = 200 * 0.95 + (consumption - 200) * 1.55;
    serviceFee = 11;
  } else if (consumption <= 650) {
    energyCost =
      200 * 0.95 +
      150 * 1.55 +
      (consumption - 350) * 1.95;
    serviceFee = 15;
  } else if (consumption <= 1000) {
    energyCost = consumption * 2.1;
    serviceFee = 25;
  } else {
    energyCost = consumption * 2.58;
    serviceFee = 40;
  }

  return {
    energyCost: Number(energyCost.toFixed(2)),
    serviceFee: Number(serviceFee.toFixed(2)),
    totalCost: Number((energyCost + serviceFee).toFixed(2)),
  };
}