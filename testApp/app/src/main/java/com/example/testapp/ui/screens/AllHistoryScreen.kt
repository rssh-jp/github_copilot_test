package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.testapp.data.*
import com.example.testapp.ui.components.CommonHeader
import com.example.testapp.ui.components.CommonScreenLayout
import java.text.SimpleDateFormat
import java.util.*

/**
 * 全履歴表示画面
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AllHistoryScreen(
    allSessions: List<GameSession>,
    onGetSessionHistory: (String) -> List<Triple<Game, List<Pair<User, Score?>>, Int>>,
    onNavigateBack: () -> Unit,
    onEditSession: (String) -> Unit,
    onStartNewGame: () -> Unit,
    onStartNewSession: () -> Unit,
    modifier: Modifier = Modifier
) {
    var showFabMenu by remember { mutableStateOf(false) }
    
    Scaffold(
        floatingActionButton = {
            Column(
                horizontalAlignment = Alignment.End
            ) {
                // 拡張メニューの表示
                if (showFabMenu) {
                    // 説明テキスト
                    Card(
                        modifier = Modifier.padding(bottom = 8.dp),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceVariant
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(12.dp)
                        ) {
                            Text(
                                text = "新しいメンバー：完全に新しいセッション",
                                fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            Text(
                                text = "過去のデータ：前回の続きから",
                                fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                    }
                    
                    // 新しいメンバーでセッション開始
                    FloatingActionButton(
                        onClick = {
                            showFabMenu = false
                            onStartNewSession()
                        },
                        modifier = Modifier.padding(bottom = 8.dp),
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            imageVector = Icons.Default.GroupAdd,
                            contentDescription = "新しいメンバーでセッション開始",
                            tint = MaterialTheme.colorScheme.onSecondary
                        )
                    }
                    
                    // 過去のデータを読み込んでゲーム開始
                    FloatingActionButton(
                        onClick = {
                            showFabMenu = false
                            onStartNewGame()
                        },
                        modifier = Modifier.padding(bottom = 8.dp),
                        containerColor = MaterialTheme.colorScheme.tertiary
                    ) {
                        Icon(
                            imageVector = Icons.Default.History,
                            contentDescription = "過去のデータでゲーム開始",
                            tint = MaterialTheme.colorScheme.onTertiary
                        )
                    }
                }
                
                // メインFAB
                FloatingActionButton(
                    onClick = {
                        if (allSessions.isEmpty()) {
                            // 履歴がない場合は直接新しいセッション開始
                            onStartNewSession()
                        } else {
                            // 履歴がある場合はメニューを表示
                            showFabMenu = !showFabMenu
                        }
                    },
                    containerColor = MaterialTheme.colorScheme.primary
                ) {
                    Icon(
                        imageVector = if (showFabMenu) Icons.Default.Close else Icons.Default.Add,
                        contentDescription = if (showFabMenu) "閉じる" else "新しいゲームを開始",
                        tint = MaterialTheme.colorScheme.onPrimary
                    )
                }
            }
        }
    ) { paddingValues ->
        CommonScreenLayout(modifier = modifier.padding(paddingValues)) {
            // ヘッダー
            CommonHeader(
                title = "得点集計",
                subtitle = if (allSessions.isEmpty()) 
                    "新しいゲームを開始しましょう" 
                else 
                    "過去のセッションを選択するか、右下のボタンから新しいゲームを開始"
            )
            
            if (allSessions.isEmpty()) {
                // 履歴がない場合 - 新しいゲーム開始
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(24.dp),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Icon(
                        imageVector = Icons.Default.SportsEsports,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "新しいゲームを開始しましょう",
                        fontSize = 18.sp,
                        color = MaterialTheme.colorScheme.onSurface,
                        fontWeight = FontWeight.Medium
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "下のボタンからゲームを開始できます",
                        fontSize = 14.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            } else {
                // セッション履歴一覧
                LazyColumn(
                    modifier = Modifier.fillMaxWidth(),
                    contentPadding = PaddingValues(16.dp),
                    verticalArrangement = Arrangement.spacedBy(16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    items(allSessions) { session ->
                        SessionHistoryCard(
                            session = session,
                            sessionHistory = onGetSessionHistory(session.id),
                            onEditClick = onEditSession,
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 8.dp)
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun SessionHistoryCard(
    session: GameSession,
    sessionHistory: List<Triple<Game, List<Pair<User, Score?>>, Int>>,
    onEditClick: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    var isExpanded by remember { mutableStateOf(false) }
    val dateFormat = remember { SimpleDateFormat("yyyy/MM/dd HH:mm", Locale.getDefault()) }
    
    val totalGames = sessionHistory.size
    val totalScores = sessionHistory.flatMap { (_, userScores, _) ->
        userScores.mapNotNull { it.second?.value }
    }.sum()
    
    Card(
        modifier = modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            // セッション情報ヘッダー
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = session.name,
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = "開始: ${dateFormat.format(Date(session.startTime))}",
                        fontSize = 12.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    if (!session.isActive && session.endTime != null) {
                        Text(
                            text = "終了: ${dateFormat.format(Date(session.endTime))}",
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                    
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    // 参加者表示
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(4.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            imageVector = Icons.Default.Group,
                            contentDescription = null,
                            modifier = Modifier.size(16.dp),
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "${session.users.size}人",
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.onSurface
                        )
                        Spacer(modifier = Modifier.width(16.dp))
                        Icon(
                            imageVector = Icons.Default.SportsEsports,
                            contentDescription = null,
                            modifier = Modifier.size(16.dp),
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "${totalGames}ゲーム",
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.onSurface
                        )
                        Spacer(modifier = Modifier.width(16.dp))
                        Text(
                            text = "合計${totalScores}点",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Medium,
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                }
                
                Row(verticalAlignment = Alignment.CenterVertically) {
                    if (session.isActive) {
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colorScheme.primary
                        ) {
                            Text(
                                text = "進行中",
                                fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onPrimary,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
                            )
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                    }
                    
                    IconButton(
                        onClick = { isExpanded = !isExpanded }
                    ) {
                        Icon(
                            imageVector = if (isExpanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                            contentDescription = if (isExpanded) "閉じる" else "開く"
                        )
                    }
                    IconButton(
                        onClick = { onEditClick(session.id) }
                    ) {
                        Icon(
                            imageVector = Icons.Default.Edit,
                            contentDescription = "編集",
                            tint = MaterialTheme.colorScheme.primary
                        )
                    }
                }
            }
            
            // 参加者名表示
            if (session.users.isNotEmpty()) {
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "参加者: ${session.users.joinToString(", ") { it.name }}",
                    fontSize = 12.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            
            // 展開時の詳細表示
            if (isExpanded && sessionHistory.isNotEmpty()) {
                Spacer(modifier = Modifier.height(16.dp))
                HorizontalDivider()
                Spacer(modifier = Modifier.height(16.dp))
                
                Text(
                    text = "ゲーム履歴",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.primary
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                sessionHistory.forEach { (game, userScores, gameTotal) ->
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceVariant
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(12.dp)
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text(
                                    text = game.name,
                                    fontSize = 14.sp,
                                    fontWeight = FontWeight.Medium
                                )
                                Text(
                                    text = "合計: ${gameTotal}点",
                                    fontSize = 14.sp,
                                    fontWeight = FontWeight.Medium,
                                    color = MaterialTheme.colorScheme.primary
                                )
                            }
                            
                            Spacer(modifier = Modifier.height(8.dp))
                            
                            userScores.forEach { (user, score) ->
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    horizontalArrangement = Arrangement.SpaceBetween
                                ) {
                                    Text(
                                        text = user.name,
                                        fontSize = 12.sp
                                    )
                                    Text(
                                        text = "${score?.value ?: 0}点",
                                        fontSize = 12.sp,
                                        fontWeight = FontWeight.Medium
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
