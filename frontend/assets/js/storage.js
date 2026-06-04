export function loadJson(key, fallbackValue) {
  try {
    const raw = window.localStorage.getItem(key);
    return raw ? JSON.parse(raw) : fallbackValue;
  } catch (error) {
    return fallbackValue;
  }
}

export function saveJson(key, value) {
  window.localStorage.setItem(key, JSON.stringify(value));
}

export function removeValue(key) {
  window.localStorage.removeItem(key);
}
