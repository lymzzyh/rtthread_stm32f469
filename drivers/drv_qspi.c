#include <board.h>

static QSPI_HandleTypeDef QSPIHandle;

static void QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
static void QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi);
static void QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi);


int drv_qspi_hw_init(void)
{
    QSPI_CommandTypeDef      sCommand;
    QSPI_MemoryMappedTypeDef sMemMappedCfg;

    QSPIHandle.Instance = QUADSPI;
    HAL_QSPI_DeInit(&QSPIHandle);

    QSPIHandle.Init.ClockPrescaler     = 1;
    QSPIHandle.Init.FifoThreshold      = 4;
    QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    QSPIHandle.Init.FlashSize          = QSPI_FLASH_SIZE;
    QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    QSPIHandle.Init.FlashID            = QSPI_FLASH_ID_1;
    QSPIHandle.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

    if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
    {
        return -1;
    }

    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    QSPI_DummyCyclesCfg(&QSPIHandle);

    sCommand.Instruction = QUAD_OUT_FAST_READ_CMD;
    sCommand.DummyCycles = DUMMY_CLOCK_CYCLES_READ_QUAD;
    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_QSPI_MemoryMapped(&QSPIHandle, &sCommand, &sMemMappedCfg) != HAL_OK)
    {
        return -1;
    }
    return 0;
}
INIT_BOARD_EXPORT(drv_qspi_hw_init);

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    static DMA_HandleTypeDef hdma;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable the QuadSPI memory interface clock */
    QSPI_CLK_ENABLE();
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();
    /* Enable GPIO clocks */
    QSPI_CS_GPIO_CLK_ENABLE();
    QSPI_CLK_GPIO_CLK_ENABLE();
    QSPI_D0_GPIO_CLK_ENABLE();
    QSPI_D1_GPIO_CLK_ENABLE();
    QSPI_D2_GPIO_CLK_ENABLE();
    QSPI_D3_GPIO_CLK_ENABLE();
    /* Enable DMA clock */
    QSPI_DMA_CLK_ENABLE();   

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* QSPI CS GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_CS_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QSPI;
    HAL_GPIO_Init(QSPI_CS_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI CLK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_CLK_PIN;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF9_QSPI;
    HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D0 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D0_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF10_QSPI;
    HAL_GPIO_Init(QSPI_D0_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D1 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D1_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF10_QSPI;
    HAL_GPIO_Init(QSPI_D1_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D2 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D2_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF9_QSPI;
    HAL_GPIO_Init(QSPI_D2_GPIO_PORT, &GPIO_InitStruct);

    /* QSPI D3 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = QSPI_D3_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF9_QSPI;
    HAL_GPIO_Init(QSPI_D3_GPIO_PORT, &GPIO_InitStruct);

    /*##-3- Configure the NVIC for QSPI #########################################*/
    /* NVIC configuration for QSPI interrupt */
    HAL_NVIC_SetPriority(QUADSPI_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);

    /*##-4- Configure the DMA channel ###########################################*/
    /* QSPI DMA channel configuration */
    hdma.Init.Channel             = QSPI_DMA_CHANNEL;                     
    hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma.Init.MemInc              = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma.Init.Mode                = DMA_NORMAL;
    hdma.Init.Priority            = DMA_PRIORITY_LOW;
    hdma.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;        /* FIFO mode disabled     */
    hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma.Init.MemBurst            = DMA_MBURST_SINGLE;           /* Memory burst           */
    hdma.Init.PeriphBurst         = DMA_PBURST_SINGLE;           /* Peripheral burst       */
    hdma.Instance                 = QSPI_DMA_INSTANCE;

    __HAL_LINKDMA(hqspi, hdma, hdma);
    HAL_DMA_Init(&hdma);

    /* NVIC configuration for DMA interrupt */
    HAL_NVIC_SetPriority(QSPI_DMA_IRQ, 0x00, 0);
    HAL_NVIC_EnableIRQ(QSPI_DMA_IRQ);
}

/**
  * @brief QSPI MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, DMA and NVIC configuration to their default state
  * @param hqspi: QSPI handle pointer
  * @retval None
  */
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *hqspi)
{
    static DMA_HandleTypeDef hdma;

    /*##-1- Disable the NVIC for QSPI and DMA ##################################*/
    HAL_NVIC_DisableIRQ(QSPI_DMA_IRQ);
    HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

    /*##-2- Disable peripherals ################################################*/
    /* De-configure DMA channel */
    hdma.Instance = QSPI_DMA_INSTANCE;
    HAL_DMA_DeInit(&hdma);
    /* De-Configure QSPI pins */
    HAL_GPIO_DeInit(QSPI_CS_GPIO_PORT, QSPI_CS_PIN);
    HAL_GPIO_DeInit(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN);
    HAL_GPIO_DeInit(QSPI_D0_GPIO_PORT, QSPI_D0_PIN);
    HAL_GPIO_DeInit(QSPI_D1_GPIO_PORT, QSPI_D1_PIN);
    HAL_GPIO_DeInit(QSPI_D2_GPIO_PORT, QSPI_D2_PIN);
    HAL_GPIO_DeInit(QSPI_D3_GPIO_PORT, QSPI_D3_PIN);

    /*##-3- Reset peripherals ##################################################*/
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();

    /* Disable the QuadSPI memory interface clock */
    QSPI_CLK_DISABLE();
}

int qspi_erase(uint32_t sector_address)
{
    QSPI_CommandTypeDef      sCommand;
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    sCommand.Instruction = SUBSECTOR_ERASE_CMD;
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    sCommand.Address     = sector_address;
    sCommand.DataMode    = QSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    if (HAL_QSPI_Command_IT(&QSPIHandle, &sCommand) != HAL_OK)
    {
        return -1;
    }
    QSPI_AutoPollingMemReady(&QSPIHandle);
    return 0;
}

size_t qspi_program(uint32_t address, const void * buffer, size_t length)
{
    QSPI_CommandTypeDef      sCommand;
    QSPI_MemoryMappedTypeDef sMemMappedCfg;

    QSPI_WriteEnable(&QSPIHandle);

    sCommand.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

    sCommand.Instruction        = QUAD_IN_FAST_PROG_CMD;
    sCommand.AddressMode        = QSPI_ADDRESS_1_LINE;
    sCommand.Address            = buffer;
    sCommand.DataMode           = QSPI_DATA_4_LINES;
    sCommand.NbData             = length;

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    if (HAL_QSPI_Transmit_DMA(&QSPIHandle, flash_addr) != HAL_OK)
    {
        return 0;
    }

    QSPI_AutoPollingMemReady(&QSPIHandle);
    QSPI_DummyCyclesCfg(&QSPIHandle);

    sCommand.Instruction = QUAD_OUT_FAST_READ_CMD;
    sCommand.DummyCycles = DUMMY_CLOCK_CYCLES_READ_QUAD;
    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_QSPI_MemoryMapped(&QSPIHandle, &sCommand, &sMemMappedCfg) != HAL_OK)
    {
        return -1;
    }
    return length;
}

static void QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef     sCommand;
    QSPI_AutoPollingTypeDef sConfig;

    /* Enable write operations ------------------------------------------ */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = WRITE_ENABLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }

    /* Configure automatic polling mode to wait for write enabling ---- */  
    sConfig.Match           = 0x02;
    sConfig.Mask            = 0x02;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    sCommand.Instruction    = READ_STATUS_REG_CMD;
    sCommand.DataMode       = QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        //TODO:
        return;
    }
}

static void QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef     sCommand;
    QSPI_AutoPollingTypeDef sConfig;

    /* Configure automatic polling mode to wait for memory ready ------ */  
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_STATUS_REG_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    sConfig.Match           = 0x00;
    sConfig.Mask            = 0x01;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling_IT(&QSPIHandle, &sCommand, &sConfig) != HAL_OK)
    {
        //TODO:
        return;
    }
}

static void QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef sCommand;
    uint8_t reg;

    /* Read Volatile Configuration register --------------------------- */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_VOL_CFG_REG_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.NbData            = 1;

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }

    if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }

    /* Enable write operations ---------------------------------------- */
    QSPI_WriteEnable(&QSPIHandle);

    /* Write Volatile Configuration register (with new dummy cycles) -- */  
    sCommand.Instruction = WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(reg, 0xF0, (DUMMY_CLOCK_CYCLES_READ_QUAD << POSITION_VAL(0xF0)));
        
    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }

    if (HAL_QSPI_Transmit(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        //TODO
        return;
    }
}
