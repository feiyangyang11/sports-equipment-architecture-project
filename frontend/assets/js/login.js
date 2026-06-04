import { authApi } from "./api.js";
import { getCurrentUser, isAuthenticated, saveAuthSession } from "./auth.js";
import { redirectByRole } from "./guard.js";
import { safeErrorMessage, setFeedback } from "./ui.js";

if (isAuthenticated()) {
  redirectByRole(getCurrentUser());
}

const loginForm = document.getElementById("loginForm");
const feedback = document.getElementById("loginFeedback");

loginForm?.addEventListener("submit", async (event) => {
  event.preventDefault();
  setFeedback(feedback, "");

  const formData = new FormData(loginForm);
  const username = String(formData.get("username") || "").trim();
  const password = String(formData.get("password") || "").trim();

  if (!username || !password) {
    setFeedback(feedback, "用户名和密码都不能为空。");
    return;
  }

  try {
    const response = await authApi.login(username, password);
    saveAuthSession(response);
    setFeedback(feedback, "登录成功，正在跳转到对应工作台...", "success");
    window.setTimeout(() => redirectByRole(response.user), 420);
  } catch (error) {
    setFeedback(feedback, safeErrorMessage(error, "登录失败，请检查账号或密码。"));
  }
});
