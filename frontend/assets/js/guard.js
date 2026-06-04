import { clearAuthSession, getCurrentUser, isAuthenticated } from "./auth.js";

export function redirectByRole(user) {
  if (!user) {
    window.location.href = "./login.html";
    return;
  }

  if (user.role === "ADMIN") {
    window.location.href = "./admin-reservations.html";
    return;
  }

  window.location.href = "./student-equipment.html";
}

export function requireRole(expectedRole) {
  if (!isAuthenticated()) {
    window.location.href = "./login.html";
    throw new Error("not authenticated");
  }

  const user = getCurrentUser();
  if (!user || user.status !== "ACTIVE") {
    clearAuthSession();
    window.location.href = "./login.html";
    throw new Error("invalid user");
  }

  if (expectedRole && user.role !== expectedRole) {
    redirectByRole(user);
    throw new Error("role mismatch");
  }

  return user;
}
