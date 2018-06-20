/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package net.ssorj.quiver.duration;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.TimeUnit;

/**
 * Factory to create different implementations of {@link EpochClock} depending on the precision allowed by the OS.
 */
public final class EpochClocks {

    private static final Logger LOGGER = LoggerFactory.getLogger(EpochClocks.class);
    private static final boolean SUPPORT_MICRO_CLOCKS;

    static {
        //TODO handle any failures while loading the native libs
        final String OS_NAME = System.getProperty("os.name").toLowerCase();
        if (OS_NAME.indexOf("linux") >= 0) {
            SUPPORT_MICRO_CLOCKS = true;
        } else {
            SUPPORT_MICRO_CLOCKS = false;
            LOGGER.warn("Microseconds precision clock is not supported: will be used the millis based on in place of it");
        }
    }

    public static EpochClock vanillaMillis() {
        return System::currentTimeMillis;
    }

    private EpochClocks() {

    }

    public static boolean supportMicroClocks(){
        return SUPPORT_MICRO_CLOCKS;
    }



    /**
     * It creates a fresh new {@link EpochClock} instance that could be used safely just by 1 thread.
     */
    public static EpochClock exclusiveMicro() {
        if (SUPPORT_MICRO_CLOCKS) {
            return new JnaDirectNativeClock();
        } else {
            throw new UnsupportedOperationException("microseconds are not supported");
        }
    }
}
