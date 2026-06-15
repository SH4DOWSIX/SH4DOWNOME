package org.qtproject.example.SH4DOWNOME;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.text.InputFilter;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

public class InputDialog {

    private static final int BG_COLOR    = 0xFF1e1e1e;
    private static final int PANEL_COLOR = 0xFF333333;
    private static final int TEXT_COLOR  = 0xFFFFFFFF;

    public static void show(Activity activity, String title, String defaultText,
                            boolean numbersOnly, int accentArgb) {
        activity.runOnUiThread(() -> {
            float dens = activity.getResources().getDisplayMetrics().density;
            int dp4  = Math.round( 4 * dens);
            int dp8  = Math.round( 8 * dens);
            int dp10 = Math.round(10 * dens);
            int dp16 = Math.round(16 * dens);
            int dp36 = Math.round(36 * dens);

            // ── Card: #1e1e1e fill, #555 1dp stroke, radius 6dp ────────────
            LinearLayout card = new LinearLayout(activity);
            card.setOrientation(LinearLayout.VERTICAL);
            GradientDrawable cardBg = new GradientDrawable();
            cardBg.setColor(BG_COLOR);
            cardBg.setStroke(Math.round(dens), 0xFF555555);
            cardBg.setCornerRadius(6 * dens);
            card.setBackground(cardBg);
            card.setPadding(dp16, dp16, dp16, dp16);

            // ── Title: white 14sp bold centred ─────────────────────────────
            TextView titleTv = new TextView(activity);
            titleTv.setText(title);
            titleTv.setTextColor(TEXT_COLOR);
            titleTv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            titleTv.setTypeface(Typeface.DEFAULT_BOLD);
            titleTv.setGravity(Gravity.CENTER_HORIZONTAL);
            card.addView(titleTv, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

            // ── Input field: #333 bg radius 4, white 18sp ──────────────────
            GradientDrawable fieldBg = new GradientDrawable();
            fieldBg.setColor(PANEL_COLOR);
            fieldBg.setCornerRadius(4 * dens);
            EditText editText = new EditText(activity);
            editText.setText(defaultText);
            editText.setSelectAllOnFocus(true);
            editText.setTextColor(TEXT_COLOR);
            editText.setHighlightColor((accentArgb & 0x00FFFFFF) | 0x66000000);
            editText.setBackground(fieldBg);
            editText.setPadding(dp8, dp8, dp8, dp8);
            editText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
            editText.setImeOptions(EditorInfo.IME_ACTION_DONE);
            if (numbersOnly) {
                editText.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED);
                editText.setGravity(Gravity.CENTER);
            } else {
                editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
                editText.setGravity(Gravity.CENTER_VERTICAL);
            }
            LinearLayout.LayoutParams fieldLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            fieldLP.topMargin = dp10;
            card.addView(editText, fieldLP);

            // ── Buttons: Cancel (#555) | OK (accent), radius 4, 36dp tall ──
            GradientDrawable cancelBg = new GradientDrawable();
            cancelBg.setColor(0xFF555555);
            cancelBg.setCornerRadius(4 * dens);
            Button cancelBtn = new Button(activity);
            cancelBtn.setText("Cancel");
            cancelBtn.setTextColor(TEXT_COLOR);
            cancelBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
            cancelBtn.setAllCaps(false);
            cancelBtn.setMinimumHeight(0);
            cancelBtn.setPadding(0, 0, 0, 0);
            cancelBtn.setBackground(cancelBg);

            GradientDrawable okBg = new GradientDrawable();
            okBg.setColor(accentArgb);
            okBg.setCornerRadius(4 * dens);
            Button okBtn = new Button(activity);
            okBtn.setText("OK");
            okBtn.setTextColor(TEXT_COLOR);
            okBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
            okBtn.setAllCaps(false);
            okBtn.setMinimumHeight(0);
            okBtn.setPadding(0, 0, 0, 0);
            okBtn.setBackground(okBg);

            LinearLayout btnRow = new LinearLayout(activity);
            btnRow.setOrientation(LinearLayout.HORIZONTAL);
            LinearLayout.LayoutParams cancelLP = new LinearLayout.LayoutParams(0, dp36, 1f);
            cancelLP.rightMargin = dp4;
            LinearLayout.LayoutParams okLP = new LinearLayout.LayoutParams(0, dp36, 1f);
            okLP.leftMargin = dp4;
            btnRow.addView(cancelBtn, cancelLP);
            btnRow.addView(okBtn, okLP);
            LinearLayout.LayoutParams btnRowLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            btnRowLP.topMargin = dp10;
            card.addView(btnRow, btnRowLP);

            // ── Dialog: transparent window so card radius shows ─────────────
            AlertDialog dialog = new AlertDialog.Builder(activity)
                .setView(card)
                .create();
            if (dialog.getWindow() != null) {
                dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
                    dialog.getWindow().setSoftInputMode(
                        WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE
                        | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
                }
            }

            final boolean[] handled = { false };

            cancelBtn.setOnClickListener(v -> {
                handled[0] = true;
                dialog.dismiss();
                nativeCancelled();
            });
            okBtn.setOnClickListener(v -> {
                handled[0] = true;
                dialog.dismiss();
                nativeAccepted(editText.getText().toString());
            });
            editText.setOnEditorActionListener((v, actionId, event) -> {
                if (actionId == EditorInfo.IME_ACTION_DONE) { okBtn.performClick(); return true; }
                return false;
            });

            dialog.setOnDismissListener(d -> { if (!handled[0]) nativeCancelled(); });

            dialog.show();

            // Fix width to ~300dp to match QML popups
            if (dialog.getWindow() != null) {
                WindowManager.LayoutParams lp = dialog.getWindow().getAttributes();
                lp.width = Math.round(300 * dens);
                dialog.getWindow().setAttributes(lp);
            }

            // Focus + show keyboard
            editText.requestFocus();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                editText.post(() -> {
                    if (editText.getWindowInsetsController() != null)
                        editText.getWindowInsetsController().show(WindowInsets.Type.ime());
                });
            } else {
                InputMethodManager imm = (InputMethodManager)
                    activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(editText, InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }

    private static native void nativeAccepted(String text);
    private static native void nativeCancelled();

    // ── Bulk-add range dialog — styled to match the QML popup exactly ────────
    public static void showBulkAdd(Activity activity, int fromTempo, int accentArgb) {
        activity.runOnUiThread(() -> {
            float dens = activity.getResources().getDisplayMetrics().density;
            int dp4  = Math.round( 4 * dens);
            int dp8  = Math.round( 8 * dens);
            int dp10 = Math.round(10 * dens);
            int dp16 = Math.round(16 * dens);
            int dp36 = Math.round(36 * dens);
            int dp72 = Math.round(72 * dens);

            // ── Card: #1e1e1e fill, #555 1dp stroke, radius 6dp ────────────
            LinearLayout card = new LinearLayout(activity);
            card.setOrientation(LinearLayout.VERTICAL);
            GradientDrawable cardBg = new GradientDrawable();
            cardBg.setColor(BG_COLOR);
            cardBg.setStroke(Math.round(dens), 0xFF555555);
            cardBg.setCornerRadius(6 * dens);
            card.setBackground(cardBg);
            card.setPadding(dp16, dp16, dp16, dp16);

            // ── Title: white 14sp bold centred ─────────────────────────────
            TextView titleTv = new TextView(activity);
            titleTv.setText("Add Section Range");
            titleTv.setTextColor(TEXT_COLOR);
            titleTv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            titleTv.setTypeface(Typeface.DEFAULT_BOLD);
            titleTv.setGravity(Gravity.CENTER_HORIZONTAL);
            card.addView(titleTv, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

            // ── Subtitle: #aaa 12sp centred ────────────────────────────────
            TextView subtitleTv = new TextView(activity);
            subtitleTv.setText("From: " + fromTempo + " BPM");
            subtitleTv.setTextColor(0xFFAAAAAA);
            subtitleTv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            subtitleTv.setGravity(Gravity.CENTER_HORIZONTAL);
            LinearLayout.LayoutParams subLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            subLP.topMargin = dp10;
            card.addView(subtitleTv, subLP);

            // ── Fields: #333 bg, radius 4, white 18sp centred ──────────────
            InputFilter[] maxLen3 = { new InputFilter.LengthFilter(3) };

            GradientDrawable fieldBg1 = new GradientDrawable();
            fieldBg1.setColor(PANEL_COLOR);
            fieldBg1.setCornerRadius(4 * dens);
            EditText targetField = new EditText(activity);
            targetField.setInputType(InputType.TYPE_CLASS_NUMBER);
            targetField.setImeOptions(EditorInfo.IME_ACTION_NEXT);
            targetField.setGravity(Gravity.CENTER);
            targetField.setTextColor(TEXT_COLOR);
            targetField.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
            targetField.setPadding(dp8, dp8, dp8, dp8);
            targetField.setBackground(fieldBg1);
            targetField.setFilters(maxLen3);

            GradientDrawable fieldBg2 = new GradientDrawable();
            fieldBg2.setColor(PANEL_COLOR);
            fieldBg2.setCornerRadius(4 * dens);
            EditText stepField = new EditText(activity);
            stepField.setText("5");
            stepField.setSelectAllOnFocus(true);
            stepField.setInputType(InputType.TYPE_CLASS_NUMBER);
            stepField.setImeOptions(EditorInfo.IME_ACTION_DONE);
            stepField.setGravity(Gravity.CENTER);
            stepField.setTextColor(TEXT_COLOR);
            stepField.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
            stepField.setPadding(dp8, dp8, dp8, dp8);
            stepField.setBackground(fieldBg2);
            stepField.setFilters(maxLen3);

            // ── Target BPM row ─────────────────────────────────────────────
            LinearLayout targetRow = new LinearLayout(activity);
            targetRow.setOrientation(LinearLayout.HORIZONTAL);
            targetRow.setGravity(Gravity.CENTER_VERTICAL);
            TextView targetLabel = new TextView(activity);
            targetLabel.setText("Target BPM");
            targetLabel.setTextColor(TEXT_COLOR);
            targetLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            LinearLayout.LayoutParams tlLP = new LinearLayout.LayoutParams(dp72,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            tlLP.rightMargin = dp8;
            targetRow.addView(targetLabel, tlLP);
            targetRow.addView(targetField, new LinearLayout.LayoutParams(
                0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
            LinearLayout.LayoutParams trLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            trLP.topMargin = dp10;
            card.addView(targetRow, trLP);

            // ── BPM Step row ───────────────────────────────────────────────
            LinearLayout stepRow = new LinearLayout(activity);
            stepRow.setOrientation(LinearLayout.HORIZONTAL);
            stepRow.setGravity(Gravity.CENTER_VERTICAL);
            TextView stepLabel = new TextView(activity);
            stepLabel.setText("BPM Step");
            stepLabel.setTextColor(TEXT_COLOR);
            stepLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            LinearLayout.LayoutParams slLP = new LinearLayout.LayoutParams(dp72,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            slLP.rightMargin = dp8;
            stepRow.addView(stepLabel, slLP);
            stepRow.addView(stepField, new LinearLayout.LayoutParams(
                0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
            LinearLayout.LayoutParams srLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            srLP.topMargin = dp10;
            card.addView(stepRow, srLP);

            // ── Buttons: Cancel (#555) | Add (accent), radius 4, 36dp tall ─
            GradientDrawable cancelBg = new GradientDrawable();
            cancelBg.setColor(0xFF555555);
            cancelBg.setCornerRadius(4 * dens);
            Button cancelBtn = new Button(activity);
            cancelBtn.setText("Cancel");
            cancelBtn.setTextColor(TEXT_COLOR);
            cancelBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
            cancelBtn.setAllCaps(false);
            cancelBtn.setMinimumHeight(0);
            cancelBtn.setPadding(0, 0, 0, 0);
            cancelBtn.setBackground(cancelBg);

            GradientDrawable addBg = new GradientDrawable();
            addBg.setColor(accentArgb);
            addBg.setCornerRadius(4 * dens);
            Button addBtn = new Button(activity);
            addBtn.setText("Add");
            addBtn.setTextColor(TEXT_COLOR);
            addBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
            addBtn.setAllCaps(false);
            addBtn.setMinimumHeight(0);
            addBtn.setPadding(0, 0, 0, 0);
            addBtn.setBackground(addBg);

            LinearLayout btnRow = new LinearLayout(activity);
            btnRow.setOrientation(LinearLayout.HORIZONTAL);
            LinearLayout.LayoutParams cancelLP = new LinearLayout.LayoutParams(0, dp36, 1f);
            cancelLP.rightMargin = dp4;
            LinearLayout.LayoutParams addLP = new LinearLayout.LayoutParams(0, dp36, 1f);
            addLP.leftMargin = dp4;
            btnRow.addView(cancelBtn, cancelLP);
            btnRow.addView(addBtn, addLP);
            LinearLayout.LayoutParams btnRowLP = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
            btnRowLP.topMargin = dp10;
            card.addView(btnRow, btnRowLP);

            // ── Dialog: transparent window bg so card radius shows ──────────
            AlertDialog dialog = new AlertDialog.Builder(activity)
                .setView(card)
                .create();
            if (dialog.getWindow() != null) {
                dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
                    dialog.getWindow().setSoftInputMode(
                        WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE
                        | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
                }
            }

            // ── Button + keyboard submit logic ─────────────────────────────
            final boolean[] handled = { false };

            cancelBtn.setOnClickListener(v -> {
                handled[0] = true;
                dialog.dismiss();
                nativeCancelled();
            });

            addBtn.setOnClickListener(v -> {
                handled[0] = true;
                String ts = targetField.getText().toString().trim();
                String ss = stepField.getText().toString().trim();
                dialog.dismiss();
                try {
                    int target = Integer.parseInt(ts);
                    int step   = Integer.parseInt(ss);
                    if (target >= 20 && target <= 300 && step >= 1 && step <= 300) {
                        nativeAccepted(ts + "|" + ss);
                        return;
                    }
                } catch (NumberFormatException ignored) {}
                nativeCancelled();
            });

            // Next on targetField moves focus to stepField
            targetField.setOnEditorActionListener((v, actionId, event) -> {
                if (actionId == EditorInfo.IME_ACTION_NEXT) { stepField.requestFocus(); return true; }
                return false;
            });
            // Done on stepField submits
            stepField.setOnEditorActionListener((v, actionId, event) -> {
                if (actionId == EditorInfo.IME_ACTION_DONE) { addBtn.performClick(); return true; }
                return false;
            });

            dialog.setOnDismissListener(d -> { if (!handled[0]) nativeCancelled(); });

            dialog.show();

            // Fix width to ~300dp (matching QML)
            if (dialog.getWindow() != null) {
                WindowManager.LayoutParams lp = dialog.getWindow().getAttributes();
                lp.width = Math.round(300 * dens);
                dialog.getWindow().setAttributes(lp);
            }

            // Focus first field and show keyboard
            targetField.requestFocus();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                targetField.post(() -> {
                    if (targetField.getWindowInsetsController() != null)
                        targetField.getWindowInsetsController().show(WindowInsets.Type.ime());
                });
            } else {
                InputMethodManager imm = (InputMethodManager)
                    activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(targetField, InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }


    // Called after forceActiveFocus() has had time to establish Qt's input
    // connection. At that point getCurrentFocus() is the Qt SurfaceView with
    // an active InputConnection, so showSoftInput succeeds.
    public static void showImeForFocused(Activity activity) {
        activity.runOnUiThread(() -> {
            View target = activity.getCurrentFocus();
            if (target == null) target = activity.getWindow().getDecorView();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                if (target.getWindowInsetsController() != null)
                    target.getWindowInsetsController().show(WindowInsets.Type.ime());
            } else {
                InputMethodManager imm = (InputMethodManager)
                    activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(target, InputMethodManager.SHOW_FORCED);
            }
        });
    }
}
