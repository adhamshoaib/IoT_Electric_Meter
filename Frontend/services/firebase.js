import { initializeApp } from 'firebase/app';
import { getDatabase } from 'firebase/database';

const firebaseConfig = {
  apiKey: 'AIzaSyDC9NYNzrYuI1CDf7NHFw3pcKEkUX0Zfvo',
  authDomain: 'test2-8c525.firebaseapp.com',
  databaseURL: 'https://test2-8c525-default-rtdb.europe-west1.firebasedatabase.app',
  projectId: 'test2-8c525',
  storageBucket: 'test2-8c525.firebasestorage.app',
  messagingSenderId: '262311847972',
  appId: '1:262311847972:web:a37fd5cc3c549dbde210b1',
};

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

export { database };