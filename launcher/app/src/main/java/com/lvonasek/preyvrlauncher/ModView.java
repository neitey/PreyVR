package com.lvonasek.preyvrlauncher;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ModView extends LinearLayout {

    private OnClickListener mListener;

    private LinearLayout mLayout;
    private ImageView mIcon;
    private TextView mTitle;
    private String mFile;
    private String mMap;

    public ModView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initView();
        getAttributes(context, attrs, defStyle);
    }

    public ModView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
        getAttributes(context, attrs, 0);
    }

    public ModView(Context context) {
        super(context);
        initView();
    }

    public String getFilename() {
        return mFile;
    }

    public String getMapname() {
        return mMap;
    }

    private void initView() {
        View root = inflate(getContext(), R.layout.lv_mods_view, this);
        mTitle = root.findViewById(R.id.title);
        mIcon = root.findViewById(R.id.icon);

        mLayout = root.findViewById(R.id.item_layout);
        setOnClickListener(mListener);
    }

    private void getAttributes(Context context, AttributeSet attrs, int defStyleAttr) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ModsView, defStyleAttr, 0);

        mIcon.setImageDrawable(a.getDrawable(R.styleable.ModsView_icon));
        mTitle.setText(a.getString(R.styleable.ModsView_title));
        mFile = a.getString(R.styleable.ModsView_file);
        mMap = a.getString(R.styleable.ModsView_map);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity(context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        ViewGroup.LayoutParams layout = mIcon.getLayoutParams();
        layout.width = displayMetrics.widthPixels / 8;
        layout.height = displayMetrics.widthPixels / 8;
        mIcon.setLayoutParams(layout);
        mTitle.setTextSize(displayMetrics.widthPixels / 80);

        if (mTitle.getText().length() == 0) {
            mTitle.setVisibility(View.GONE);
        }

        a.recycle();
    }

    private Activity getActivity(Context context) {
        if (context == null) return null;
        if (context instanceof Activity) return (Activity) context;
        if (context instanceof ContextWrapper) return getActivity(((ContextWrapper)context).getBaseContext());
        return null;
    }

    @Override
    public void setOnClickListener(OnClickListener listener) {
        mListener = listener;
        if (mLayout != null) {
            mLayout.setOnClickListener(mListener);
            mTitle.setOnClickListener(mListener);
            mIcon.setOnClickListener(mListener);
        }
    }
}
