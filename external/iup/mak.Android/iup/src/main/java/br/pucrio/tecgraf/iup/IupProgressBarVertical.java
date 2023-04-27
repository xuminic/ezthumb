/*
 * This code is from
 * https://stackoverflow.com/a/9311020
 * Ramesh Akula https://stackoverflow.com/users/874752/ramesh-akula
 * Stack Overflow uses the Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) license. https://creativecommons.org/licenses/by-sa/3.0/
 */
package br.pucrio.tecgraf.iup;
import java.lang.Object;
import android.content.Context;
import android.graphics.Canvas;
import android.support.v7.view.ContextThemeWrapper;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.util.Log;
import android.widget.ProgressBar;
 
import br.pucrio.tecgraf.iup.IupApplication;
import br.pucrio.tecgraf.iup.IupCommon;

public class IupProgressBarVertical extends ProgressBar
{
    private int x, y, z, w;
/*
    @Override
    protected void drawableStateChanged() {
        // TODO Auto-generated method stub
        super.drawableStateChanged();
    }
*/
    public IupProgressBarVertical(Context context) {
        super(context);
    }

    public IupProgressBarVertical(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public IupProgressBarVertical(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(h, w, oldh, oldw);
        this.x = w;
        this.y = h;
        this.z = oldw;
        this.w = oldh;
		invalidate(); // Oliver Jonas fix: https://stackoverflow.com/users/2097259/oliver-jonas
    }

    @Override
    protected synchronized void onMeasure(int widthMeasureSpec,
            int heightMeasureSpec) {
        super.onMeasure(heightMeasureSpec, widthMeasureSpec);
        setMeasuredDimension(getMeasuredHeight(), getMeasuredWidth());
    }

    protected void onDraw(Canvas c) {
        c.rotate(-90);
        c.translate(-getHeight(), 0);
        super.onDraw(c);
    }

    // EW: This appears to be if you want to change the value by touching the bar. We don't need this.
/*
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!isEnabled()) {
            return false;
        }

        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:

            setSelected(true);
            setPressed(true);
            break;
        case MotionEvent.ACTION_MOVE:
            setProgress(getMax()
                    - (int) (getMax() * event.getY() / getHeight()));
            onSizeChanged(getWidth(), getHeight(), 0, 0);

            break;
        case MotionEvent.ACTION_UP:
            setSelected(false);
            setPressed(false);
            break;

        case MotionEvent.ACTION_CANCEL:
            break;
        }
        return true;
    }
*/
    // EW: The bounds checks are incorrect in API 26+ because ranges may be changed.
    // I'm not convinced the onSizeChanged() call is needed from my testing.
    // So I'm just disabling the whole thing for now.
/*
    @Override
    public synchronized void setProgress(int progress) {

        if (progress >= 0)
            super.setProgress(progress);

        else
            super.setProgress(0);
        onSizeChanged(x, y, z, w);

    }
*/
}

