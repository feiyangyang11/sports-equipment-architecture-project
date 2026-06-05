import { APP_NAME } from "./config.js";
import { clearAuthSession } from "./auth.js";
import { authApi } from "./api.js";
import { escapeHtml } from "./utils.js";

const navByRole = {
  STUDENT: [
    { key: "student-equipment", label: "器材总览", hint: "Browse", href: "./student-equipment.html" },
    { key: "student-reservations", label: "我的预约", hint: "Reserve", href: "./student-reservations.html" },
    { key: "student-borrows", label: "我的借用", hint: "Borrow", href: "./student-borrows.html" },
  ],
  ADMIN: [
    { key: "admin-reservations", label: "预约审核", hint: "Review", href: "./admin-reservations.html" },
    { key: "admin-borrows", label: "借还管理", hint: "Handle", href: "./admin-borrows.html" },
  ],
};

function renderNav(role, currentKey) {
  const items = navByRole[role] || [];
  return `
    <p class="nav-section-title">${role === "ADMIN" ? "管理导航" : "学生导航"}</p>
    ${items.map((item) => `
      <a class="nav-link ${item.key === currentKey ? "is-active" : ""}" href="${item.href}">
        <span>${item.label}</span>
        <span>${item.hint}</span>
      </a>
    `).join("")}
  `;
}

export function mountShell({
  user,
  role,
  navKey,
  eyebrow,
  title,
  lead,
  stats = "",
  main,
  aside = "",
  topBadge = "校园体育器材管理",
}) {
  document.body.innerHTML = `
    <div class="app-shell">
      <aside class="app-shell__sidebar">
        <section class="app-shell__brand">
          <p class="brand-eyebrow">${role === "ADMIN" ? "Admin Console" : "Student Space"}</p>
          <h1 class="brand-title">${APP_NAME}</h1>
          <p class="brand-subtitle">围绕器材预约、审核、借出与归还提供统一的业务入口。</p>
        </section>
        <nav class="app-shell__nav">
          ${renderNav(role, navKey)}
        </nav>
        <section class="profile-card">
          <p class="profile-card__name">${escapeHtml(user.realName)}</p>
          <p class="profile-card__meta">${escapeHtml(user.username)} · ${escapeHtml(user.role)}</p>
          <div class="button-row" style="margin-top: 16px;">
            <button class="btn btn--ghost btn--wide" id="logoutButton">退出登录</button>
          </div>
        </section>
      </aside>
      <main class="app-shell__main">
        <div class="topbar">
          <span class="topbar__badge">${escapeHtml(topBadge)}</span>
          <div class="topbar__actions"></div>
        </div>
        <section class="hero-card fade-up">
          <p class="hero-card__eyebrow">${escapeHtml(eyebrow)}</p>
          <h2 class="hero-card__title">${escapeHtml(title)}</h2>
          <p class="hero-card__lead">${escapeHtml(lead)}</p>
        </section>
        ${stats}
        <section class="content-grid ${aside ? "" : "content-grid--single"} fade-up">
          <section>${main}</section>
          ${aside ? `<aside>${aside}</aside>` : ""}
        </section>
      </main>
    </div>
  `;

  document.getElementById("logoutButton")?.addEventListener("click", async () => {
    try {
      await authApi.logout();
    } catch (_error) {
      // Still clear the local session even if the logout request fails.
    } finally {
      clearAuthSession();
      window.location.href = "./login.html";
    }
  });
}
