import { STORAGE_KEYS } from "./config.js";
import { createInitialMockState } from "./mock-data.js";
import { loadJson, saveJson } from "./storage.js";

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function delay(result, shouldReject = false) {
  return new Promise((resolve, reject) => {
    window.setTimeout(() => {
      if (shouldReject) {
        reject(result);
      } else {
        resolve(result);
      }
    }, 160);
  });
}

function createApiError(status, message) {
  return { status, message };
}

function getState() {
  const existing = loadJson(STORAGE_KEYS.mockState, null);
  if (existing) {
    return existing;
  }
  const initial = createInitialMockState();
  saveState(initial);
  return initial;
}

function saveState(state) {
  saveJson(STORAGE_KEYS.mockState, state);
}

function nextId(state, key) {
  const current = state.counters[key];
  state.counters[key] += 1;
  return current;
}

function formatSequence(prefix, id) {
  return `${prefix}${String(id).padStart(10, "0")}`;
}

function paginate(items, pageNo, pageSize) {
  const start = (pageNo - 1) * pageSize;
  return items.slice(start, start + pageSize);
}

function resolveToken(token) {
  const state = getState();
  const userId = state.sessions[token];
  if (!userId) {
    throw createApiError(401, "invalid or expired token");
  }
  const user = state.users.find((item) => item.id === userId);
  if (!user) {
    throw createApiError(401, "invalid or expired token");
  }
  return { state, user };
}

function sanitizeUser(user) {
  return {
    id: user.id,
    username: user.username,
    realName: user.realName,
    role: user.role,
    studentNo: user.studentNo,
    status: user.status,
  };
}

function requireRole(token, role) {
  const { state, user } = resolveToken(token);
  if (user.status !== "ACTIVE") {
    throw createApiError(403, "user is disabled");
  }
  if (role && user.role !== role) {
    throw createApiError(403, `${role.toLowerCase()} role is required`);
  }
  return { state, user };
}

function getEquipmentById(state, equipmentId) {
  return state.equipment.find((item) => item.id === equipmentId);
}

function getReservationById(state, reservationId) {
  return state.reservations.find((item) => item.id === reservationId);
}

function getBorrowById(state, borrowId) {
  return state.borrowRecords.find((item) => item.id === borrowId);
}

function sortByIdDesc(items) {
  return [...items].sort((left, right) => right.id - left.id);
}

export async function login({ username, password }) {
  const state = getState();
  const user = state.users.find(
      (item) => item.username === username && item.password === password,
  );
  if (!user) {
    return delay(createApiError(401, "invalid username or password"), true);
  }

  const token = `mock-token-${state.counters.token++}-${user.id}`;
  state.sessions[token] = user.id;
  saveState(state);
  return delay({
    token,
    user: sanitizeUser(user),
  });
}

export async function getMe({ token }) {
  try {
    const { user } = requireRole(token);
    return delay(sanitizeUser(user));
  } catch (error) {
    return delay(error, true);
  }
}

export async function logout({ token }) {
  try {
    const { state } = resolveToken(token);
    delete state.sessions[token];
    saveState(state);
    return delay({ message: "logout success" });
  } catch (error) {
    return delay(error, true);
  }
}

export async function getEquipmentCategories() {
  const state = getState();
  return delay({ items: clone(state.equipmentCategories) });
}

export async function getEquipmentPage({ pageNo, pageSize, categoryId }) {
  const state = getState();
  const filtered = state.equipment.filter((item) => {
    if (!categoryId) {
      return true;
    }
    return item.categoryId === Number(categoryId);
  });

  return delay({
    pageNo,
    pageSize,
    total: filtered.length,
    items: clone(paginate(filtered, pageNo, pageSize)),
  });
}

export async function getEquipmentByIdApi({ id }) {
  const state = getState();
  const equipment = getEquipmentById(state, Number(id));
  if (!equipment) {
    return delay(createApiError(404, "equipment not found"), true);
  }
  return delay(clone(equipment));
}

export async function createReservation({ token, payload }) {
  try {
    const { state, user } = requireRole(token, "STUDENT");
    const equipment = getEquipmentById(state, Number(payload.equipmentId));
    if (!equipment) {
      throw createApiError(404, "equipment not found");
    }
    if (equipment.status !== "AVAILABLE") {
      throw createApiError(409, "equipment is not available");
    }
    if (!payload.quantity || Number(payload.quantity) <= 0) {
      throw createApiError(400, "quantity must be > 0");
    }
    if (Number(payload.quantity) > equipment.availableStock) {
      throw createApiError(409, "requested quantity exceeds available stock");
    }
    if (!payload.reservationStartAt || !payload.reservationEndAt) {
      throw createApiError(400, "reservation time is required");
    }

    const reservationId = nextId(state, "reservation");
    const reservation = {
      id: reservationId,
      reservationNo: formatSequence("RES", reservationId),
      studentUserId: user.id,
      studentUsername: user.username,
      studentRealName: user.realName,
      equipmentId: equipment.id,
      equipmentCode: equipment.equipmentCode,
      equipmentName: equipment.equipmentName,
      reservationStartAt: payload.reservationStartAt,
      reservationEndAt: payload.reservationEndAt,
      quantity: Number(payload.quantity),
      status: "PENDING",
      requestNote: payload.requestNote || "",
      reviewNote: "",
      cancelReason: "",
      reviewedAt: "",
      createdAt: state.counters.now,
      updatedAt: state.counters.now,
    };

    state.reservations.push(reservation);
    saveState(state);
    return delay(clone(reservation));
  } catch (error) {
    return delay(error, true);
  }
}

function filterReservationsByStatus(items, status) {
  if (!status) {
    return items;
  }
  return items.filter((item) => item.status === status);
}

export async function getMyReservations({ token, pageNo, pageSize, status }) {
  try {
    const { user, state } = requireRole(token, "STUDENT");
    const items = filterReservationsByStatus(
        sortByIdDesc(
            state.reservations.filter((item) => item.studentUserId === user.id),
        ),
        status,
    );
    return delay({
      pageNo,
      pageSize,
      total: items.length,
      items: clone(paginate(items, pageNo, pageSize)),
    });
  } catch (error) {
    return delay(error, true);
  }
}

export async function getMyReservationById({ token, id }) {
  try {
    const { user, state } = requireRole(token, "STUDENT");
    const item = state.reservations.find(
        (reservation) =>
          reservation.id === Number(id) && reservation.studentUserId === user.id,
    );
    if (!item) {
      throw createApiError(404, "reservation not found");
    }
    return delay(clone(item));
  } catch (error) {
    return delay(error, true);
  }
}

export async function cancelMyReservation({ token, id, payload }) {
  try {
    const { user, state } = requireRole(token, "STUDENT");
    const item = state.reservations.find(
        (reservation) =>
          reservation.id === Number(id) && reservation.studentUserId === user.id,
    );
    if (!item) {
      throw createApiError(404, "reservation not found");
    }
    if (!["PENDING", "APPROVED"].includes(item.status)) {
      throw createApiError(409, "only PENDING or APPROVED reservations can be canceled");
    }
    item.status = "CANCELED";
    item.cancelReason = payload.cancelReason || "";
    item.updatedAt = state.counters.now;
    saveState(state);
    return delay(clone(item));
  } catch (error) {
    return delay(error, true);
  }
}

export async function getAdminReservations({ token, pageNo, pageSize, status }) {
  try {
    const { state } = requireRole(token, "ADMIN");
    const items = filterReservationsByStatus(sortByIdDesc(state.reservations), status);
    return delay({
      pageNo,
      pageSize,
      total: items.length,
      items: clone(paginate(items, pageNo, pageSize)),
    });
  } catch (error) {
    return delay(error, true);
  }
}

export async function reviewReservation({ token, id, action, payload }) {
  try {
    requireRole(token, "ADMIN");
    const state = getState();
    const item = getReservationById(state, Number(id));
    if (!item) {
      throw createApiError(404, "reservation not found");
    }
    if (item.status !== "PENDING") {
      throw createApiError(409, "only PENDING reservations can be reviewed");
    }
    item.status = action === "approve" ? "APPROVED" : "REJECTED";
    item.reviewNote = payload.reviewNote || "";
    item.reviewedAt = state.counters.now;
    item.updatedAt = state.counters.now;
    saveState(state);
    return delay(clone(item));
  } catch (error) {
    return delay(error, true);
  }
}

function filterBorrowsByStatus(items, status) {
  if (!status) {
    return items;
  }
  return items.filter((item) => item.status === status);
}

export async function createBorrow({ token, payload }) {
  try {
    const { state, user } = requireRole(token, "ADMIN");
    const reservation = getReservationById(state, Number(payload.reservationId));
    if (!reservation) {
      throw createApiError(404, "reservation not found");
    }
    if (reservation.status !== "APPROVED") {
      throw createApiError(409, "only APPROVED reservations can be borrowed");
    }

    const equipment = getEquipmentById(state, reservation.equipmentId);
    if (!equipment) {
      throw createApiError(404, "equipment not found");
    }
    if (reservation.quantity > equipment.availableStock) {
      throw createApiError(409, "available stock is insufficient");
    }

    const borrowId = nextId(state, "borrow");
    const borrowRecord = {
      id: borrowId,
      borrowNo: formatSequence("BOR", borrowId),
      reservationId: reservation.id,
      reservationNo: reservation.reservationNo,
      studentUserId: reservation.studentUserId,
      studentUsername: reservation.studentUsername,
      studentRealName: reservation.studentRealName,
      equipmentId: reservation.equipmentId,
      equipmentCode: reservation.equipmentCode,
      equipmentName: reservation.equipmentName,
      quantity: reservation.quantity,
      borrowedBy: user.id,
      borrowedAt: payload.borrowedAt,
      dueAt: payload.dueAt,
      receivedBy: 0,
      returnedAt: "",
      status: "BORROWING",
      returnNote: "",
      createdAt: state.counters.now,
      updatedAt: state.counters.now,
    };

    reservation.status = "BORROWED";
    reservation.updatedAt = state.counters.now;
    equipment.availableStock -= reservation.quantity;
    state.borrowRecords.push(borrowRecord);
    saveState(state);
    return delay(clone(borrowRecord));
  } catch (error) {
    return delay(error, true);
  }
}

export async function getMyBorrows({ token, pageNo, pageSize, status }) {
  try {
    const { user, state } = requireRole(token, "STUDENT");
    const items = filterBorrowsByStatus(
        sortByIdDesc(
            state.borrowRecords.filter((item) => item.studentUserId === user.id),
        ),
        status,
    );
    return delay({
      pageNo,
      pageSize,
      total: items.length,
      items: clone(paginate(items, pageNo, pageSize)),
    });
  } catch (error) {
    return delay(error, true);
  }
}

export async function getMyBorrowById({ token, id }) {
  try {
    const { user, state } = requireRole(token, "STUDENT");
    const item = state.borrowRecords.find(
        (borrow) => borrow.id === Number(id) && borrow.studentUserId === user.id,
    );
    if (!item) {
      throw createApiError(404, "borrow record not found");
    }
    return delay(clone(item));
  } catch (error) {
    return delay(error, true);
  }
}

export async function getAdminBorrows({ token, pageNo, pageSize, status }) {
  try {
    requireRole(token, "ADMIN");
    const state = getState();
    const items = filterBorrowsByStatus(sortByIdDesc(state.borrowRecords), status);
    return delay({
      pageNo,
      pageSize,
      total: items.length,
      items: clone(paginate(items, pageNo, pageSize)),
    });
  } catch (error) {
    return delay(error, true);
  }
}

export async function getAdminBorrowById({ token, id }) {
  try {
    requireRole(token, "ADMIN");
    const state = getState();
    const item = getBorrowById(state, Number(id));
    if (!item) {
      throw createApiError(404, "borrow record not found");
    }
    return delay(clone(item));
  } catch (error) {
    return delay(error, true);
  }
}

export async function returnBorrow({ token, id, payload }) {
  try {
    const { state, user } = requireRole(token, "ADMIN");
    const borrowRecord = getBorrowById(state, Number(id));
    if (!borrowRecord) {
      throw createApiError(404, "borrow record not found");
    }
    if (borrowRecord.status !== "BORROWING") {
      throw createApiError(409, "only BORROWING borrow records can be returned");
    }

    const reservation = getReservationById(state, borrowRecord.reservationId);
    const equipment = getEquipmentById(state, borrowRecord.equipmentId);
    if (!reservation || !equipment) {
      throw createApiError(404, "related record not found");
    }

    borrowRecord.receivedBy = user.id;
    borrowRecord.returnedAt = payload.returnedAt;
    borrowRecord.returnNote = payload.returnNote || "";
    borrowRecord.status = "RETURNED";
    borrowRecord.updatedAt = state.counters.now;
    reservation.status = "COMPLETED";
    reservation.updatedAt = state.counters.now;
    equipment.availableStock += borrowRecord.quantity;
    saveState(state);
    return delay(clone(borrowRecord));
  } catch (error) {
    return delay(error, true);
  }
}
