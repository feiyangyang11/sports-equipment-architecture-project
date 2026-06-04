export function setFeedback(element, message, type = "error") {
  if (!element) {
    return;
  }

  if (!message) {
    element.textContent = "";
    element.className = "feedback";
    return;
  }

  element.textContent = message;
  element.className = `feedback feedback--${type} is-visible`;
}

export function safeErrorMessage(error, fallbackMessage) {
  return error?.message || fallbackMessage;
}

export function bootstrapModal(id) {
  const element = document.getElementById(id);
  if (!element || !window.bootstrap) {
    return null;
  }
  return window.bootstrap.Modal.getOrCreateInstance(element);
}
