import { STORAGE_KEYS } from "./config.js";
import { loadJson, removeValue, saveJson } from "./storage.js";

export function saveAuthSession(session) {
  saveJson(STORAGE_KEYS.auth, session);
}

export function getAuthSession() {
  return loadJson(STORAGE_KEYS.auth, null);
}

export function getToken() {
  return getAuthSession()?.token || "";
}

export function getCurrentUser() {
  return getAuthSession()?.user || null;
}

export function isAuthenticated() {
  return Boolean(getToken() && getCurrentUser());
}

export function clearAuthSession() {
  removeValue(STORAGE_KEYS.auth);
}
