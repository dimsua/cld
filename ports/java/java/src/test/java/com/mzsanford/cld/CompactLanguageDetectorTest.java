/**
 *
 */
package com.mzsanford.cld;

import static org.junit.Assert.*;

import org.junit.Test;

/**
 * @author matt
 *
 */
public class CompactLanguageDetectorTest {

    /**
     * Verify that the loadLibrary call is working correctly.
     */
    @Test
    public void testConstructor() {
        CompactLanguageDetector compactLanguageDetector = new CompactLanguageDetector();
        assertNotNull(compactLanguageDetector);
    }

    @Test(expected=IllegalArgumentException.class)
    public void testDetectNull() {
        CompactLanguageDetector compactLanguageDetector = new CompactLanguageDetector();
        compactLanguageDetector.detect(null);
    }

}
